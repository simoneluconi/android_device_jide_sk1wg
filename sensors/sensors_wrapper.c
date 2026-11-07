/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * sensors.sk1wg: presents stock Remix OS's sensors.macallan.so, a device
 * API 0.1 HAL (activate/setDelay/poll only), as a 1.3 one, the minimum
 * android.hardware.sensors@1.0-impl accepts ("Check failed:
 * getHalDeviceVersion() >= SENSORS_DEVICE_API_VERSION_1_3"):
 *   - batch() becomes setDelay() (no hardware FIFO, so no batching);
 *   - flush() queues the META_DATA_FLUSH_COMPLETE event the framework
 *     waits for and poll() returns it before the next sensor events.
 * Selected with ro.hardware.sensors=sk1wg (device.mk).
 */

#define LOG_TAG "sensors.sk1wg"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>
#include <log/log.h>

#define STOCK_HAL "/vendor/lib/hw/sensors.macallan.so"
#define MAX_PENDING_FLUSHES 64

static struct sensors_module_t *g_stock_module;
static struct sensors_poll_device_t *g_stock_dev;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_pending[MAX_PENDING_FLUSHES];
static int g_npending;

static int load_stock(void)
{
    void *lib;

    if (g_stock_module)
        return 0;
    lib = dlopen(STOCK_HAL, RTLD_NOW);
    if (!lib) {
        ALOGE("dlopen %s: %s", STOCK_HAL, dlerror());
        return -ENOENT;
    }
    g_stock_module = (struct sensors_module_t *)dlsym(lib, HAL_MODULE_INFO_SYM_AS_STR);
    if (!g_stock_module) {
        ALOGE("%s has no %s", STOCK_HAL, HAL_MODULE_INFO_SYM_AS_STR);
        dlclose(lib);
        return -EINVAL;
    }
    return 0;
}

static int wrap_get_sensors_list(struct sensors_module_t *module __unused,
                                 struct sensor_t const **list)
{
    if (load_stock())
        return 0;
    return g_stock_module->get_sensors_list(g_stock_module, list);
}

static int wrap_activate(struct sensors_poll_device_t *dev __unused, int handle, int enabled)
{
    return g_stock_dev->activate(g_stock_dev, handle, enabled);
}

static int wrap_set_delay(struct sensors_poll_device_t *dev __unused, int handle, int64_t ns)
{
    return g_stock_dev->setDelay(g_stock_dev, handle, ns);
}

static int wrap_batch(struct sensors_poll_device_1 *dev __unused, int handle, int flags __unused,
                      int64_t period_ns, int64_t timeout __unused)
{
    return g_stock_dev->setDelay(g_stock_dev, handle, period_ns);
}

static int wrap_flush(struct sensors_poll_device_1 *dev __unused, int handle)
{
    int ret = 0;

    pthread_mutex_lock(&g_lock);
    if (g_npending < MAX_PENDING_FLUSHES)
        g_pending[g_npending++] = handle;
    else
        ret = -ENOMEM;
    pthread_mutex_unlock(&g_lock);
    return ret;
}

static int wrap_poll(struct sensors_poll_device_t *dev __unused, sensors_event_t *data, int count)
{
    int n = 0;

    pthread_mutex_lock(&g_lock);
    while (g_npending > 0 && n < count) {
        sensors_event_t *ev = &data[n++];

        memset(ev, 0, sizeof(*ev));
        ev->version = META_DATA_VERSION;
        ev->type = SENSOR_TYPE_META_DATA;
        ev->meta_data.what = META_DATA_FLUSH_COMPLETE;
        ev->meta_data.sensor = g_pending[--g_npending];
    }
    pthread_mutex_unlock(&g_lock);
    if (n > 0)
        return n;

    return g_stock_dev->poll(g_stock_dev, data, count);
}

static int wrap_close(struct hw_device_t *dev)
{
    int ret = 0;

    if (g_stock_dev) {
        ret = g_stock_dev->common.close(&g_stock_dev->common);
        g_stock_dev = NULL;
    }
    free(dev);
    return ret;
}

static int wrap_open(const struct hw_module_t *module, const char *id,
                     struct hw_device_t **device)
{
    struct sensors_poll_device_1 *dev;
    int ret;

    if (strcmp(id, SENSORS_HARDWARE_POLL))
        return -EINVAL;
    ret = load_stock();
    if (ret)
        return ret;
    ret = g_stock_module->common.methods->open(&g_stock_module->common, id,
                                               (struct hw_device_t **)&g_stock_dev);
    if (ret || !g_stock_dev) {
        ALOGE("stock open failed: %d", ret);
        return ret ? ret : -ENODEV;
    }
    ALOGI("stock HAL device version 0x%x, presented as 1.3", g_stock_dev->common.version);

    dev = calloc(1, sizeof(*dev));
    if (!dev)
        return -ENOMEM;
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = SENSORS_DEVICE_API_VERSION_1_3;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = wrap_close;
    dev->activate = wrap_activate;
    dev->setDelay = wrap_set_delay;
    dev->poll = wrap_poll;
    dev->batch = wrap_batch;
    dev->flush = wrap_flush;

    *device = &dev->common;
    return 0;
}

static struct hw_module_methods_t wrap_methods = {
    .open = wrap_open,
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SENSORS_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "sk1wg sensors (stock macallan HAL, 0.1 as 1.3)",
        .author = "The JideRemixTabletRevived Project",
        .methods = &wrap_methods,
    },
    .get_sensors_list = wrap_get_sensors_list,
};

/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * Stand-ins for NVIDIA's libnvcpl.so and libcplconnectclient.so.
 *
 * The stock ones ask NVIDIA's Control Panel service (absent here) for
 * per-app profile settings through Android 5.1's C++ binder API, with an
 * android::Parcel on their own stack. Android 14's Parcel is bigger than
 * the space they reserve, so its constructor overwrites the caller's saved
 * registers: SurfaceFlinger crashed in libGLESv2_tegra_impl.so right after
 * NvCplGetAppProfileSettingInt() returned (fault at 0x10001, the clobbered
 * __stack_chk_guard pointer).
 *
 * Every caller in the GLES/EGL/nvos blobs only checks for 0 (success) and
 * falls back to its defaults otherwise, so these report "not available".
 * The extra arguments are ignored (AAPCS: the callee may take fewer).
 */

#define NVCPL_NOT_AVAILABLE 1

int NvCplGetAppProfileSettingInt(const char *app, int setting, int *value)
{
    (void)app; (void)setting; (void)value;
    return NVCPL_NOT_AVAILABLE;
}

int NvCplGetAppProfileSettingString(const char *app, int setting, char *buf, int len)
{
    (void)app; (void)setting; (void)buf; (void)len;
    return NVCPL_NOT_AVAILABLE;
}

int NvCplGetAppProfileSetting3DVStruct(const char *app, int setting, void *out)
{
    (void)app; (void)setting; (void)out;
    return NVCPL_NOT_AVAILABLE;
}

#ifdef NVCPL_FULL
/* Only libnvcpl.so exports these */
int NvCplGetOpenGlAppProfileSettingInt(const char *app, int setting, int *value)
{
    (void)app; (void)setting; (void)value;
    return NVCPL_NOT_AVAILABLE;
}

int NvCplGetOpenGlAppProfileSettingString(const char *app, int setting, char *buf, int len)
{
    (void)app; (void)setting; (void)buf; (void)len;
    return NVCPL_NOT_AVAILABLE;
}

int NvCplSetSysProperty(const char *name, const char *value)
{
    (void)name; (void)value;
    return NVCPL_NOT_AVAILABLE;
}

int NvCplInject(const void *data, int len)
{
    (void)data; (void)len;
    return NVCPL_NOT_AVAILABLE;
}
#endif

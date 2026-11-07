/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * AIDL Lights HAL for the sk1wg: only the LCD backlight, the PWM backlight
 * of the macallan board (/sys/class/backlight/pwm-backlight). The tablet
 * has no notification, button or battery LEDs.
 */

#define LOG_TAG "android.hardware.light-service.sk1wg"

#include <aidl/android/hardware/light/BnLights.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <mutex>
#include <string>

using aidl::android::hardware::light::BnLights;
using aidl::android::hardware::light::HwLight;
using aidl::android::hardware::light::HwLightState;
using aidl::android::hardware::light::LightType;

namespace {

const std::string kBacklight = "/sys/class/backlight/pwm-backlight/brightness";
const std::string kMaxBacklight = "/sys/class/backlight/pwm-backlight/max_brightness";
constexpr int32_t kBacklightId = 0;

uint32_t readMaxBrightness() {
    std::string value;
    uint32_t max = 0;
    if (!android::base::ReadFileToString(kMaxBacklight, &value) ||
        !android::base::ParseUint(android::base::Trim(value), &max) || max == 0) {
        LOG(WARNING) << "cannot read " << kMaxBacklight << ", assuming 255";
        return 255;
    }
    return max;
}

// ARGB colour to 0..255 brightness, as the framework's own conversion
uint32_t colorToBrightness(int32_t color) {
    const uint32_t c = static_cast<uint32_t>(color);
    return ((77 * ((c >> 16) & 0xff)) + (150 * ((c >> 8) & 0xff)) + (29 * (c & 0xff))) >> 8;
}

class Lights : public BnLights {
  public:
    Lights() : mMaxBrightness(readMaxBrightness()) {}

    ndk::ScopedAStatus setLightState(int32_t id, const HwLightState& state) override {
        if (id != kBacklightId) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
        const uint32_t level = colorToBrightness(state.color) * mMaxBrightness / 255;
        std::lock_guard<std::mutex> lock(mLock);
        if (!android::base::WriteStringToFile(std::to_string(level), kBacklight)) {
            PLOG(ERROR) << "cannot write " << kBacklight;
            return ndk::ScopedAStatus::fromExceptionCode(EX_SERVICE_SPECIFIC);
        }
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus getLights(std::vector<HwLight>* lights) override {
        HwLight backlight;
        backlight.id = kBacklightId;
        backlight.ordinal = 0;
        backlight.type = LightType::BACKLIGHT;
        lights->push_back(backlight);
        return ndk::ScopedAStatus::ok();
    }

  private:
    const uint32_t mMaxBrightness;
    std::mutex mLock;
};

}  // namespace

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    std::shared_ptr<Lights> lights = ndk::SharedRefBase::make<Lights>();
    const std::string instance = std::string() + Lights::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(lights->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not be reached
}

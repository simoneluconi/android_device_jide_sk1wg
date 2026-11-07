#
# SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
# SPDX-License-Identifier: Apache-2.0
#

# Version name: 21-<date>-UNOFFICIAL-JideRemixTabletRevived-sk1wg
TARGET_UNOFFICIAL_BUILD_ID := JideRemixTabletRevived

PRODUCT_RELEASE_NAME := sk1wg

# Inherit device configuration
$(call inherit-product, device/jide/sk1wg/device.mk)

# Wi-Fi only tablet: no telephony. 32-bit only (Tegra 4 is ARMv7).
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base.mk)

# Boot animation
TARGET_SCREEN_HEIGHT := 1080
TARGET_SCREEN_WIDTH := 1920

# Bring-up: adb on from the first boot, without the key prompt (the
# display may not work yet). vendor/lineage/config/common.mk turns it into
# ro.adb.secure=0, which also makes the build add persist.sys.usb.config=adb;
# set here for userdebug/eng only.
# TODO: drop once the display and the setup wizard work.
ifneq ($(TARGET_BUILD_VARIANT),user)
WITH_ADB_INSECURE := true
endif

# LineageOS: the "mini" Wi-Fi tablet set (fewer bundled apps; the 1000 MB
# system partition is tight)
$(call inherit-product, vendor/lineage/config/common_mini_tablet_wifionly.mk)

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := sk1wg
PRODUCT_NAME := lineage_sk1wg
PRODUCT_BRAND := Jide
PRODUCT_MODEL := REMIX SK1WG
PRODUCT_MANUFACTURER := JIDE

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=sk1wg \
    TARGET_DEVICE=sk1wg \
    PRIVATE_BUILD_DESC="sk1wg-user 5.1.1 LMY47V B2016061701 release-keys"

BUILD_FINGERPRINT := Jide/sk1wg/sk1wg:5.1.1/LMY47V/B2016061701:user/release-keys

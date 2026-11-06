#
# SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
# SPDX-License-Identifier: Apache-2.0
#
# Product configuration for the Jide Remix Ultra Tablet (REMIX SK1WG),
# NVIDIA Tegra 4 (T114) on the "macallan" reference board.
#
# First bring-up set: display (NVIDIA's GLES, gralloc and hwcomposer
# through the HIDL passthrough adapters), adb, health, keymaster and
# gatekeeper (software), a stub audio HAL. Wi-Fi, Bluetooth, sensors and
# real audio come next (docs/TODO.md).
#

LOCAL_PATH := device/jide/sk1wg

# Tablet, 32-bit only
PRODUCT_CHARACTERISTICS := tablet
PRODUCT_AAPT_CONFIG := normal large xlarge hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# Overlays
DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# Shipping API: the device launched with Android 5.1 (API 22). The legacy
# HALs below need the relaxed VINTF checks LineageOS-UL keeps for that.
PRODUCT_SHIPPING_API_LEVEL := 22

# Dalvik: 2 GB of RAM
$(call inherit-product, frameworks/native/build/tablet-10in-xhdpi-2048-dalvik-heap.mk)

# Init and fstab. SK1WG_SDCARD_ROOT=true builds a test variant that runs
# entirely from the microSD card (rootdir/etc/fstab.macallan.sdcard).
ifeq ($(SK1WG_SDCARD_ROOT),true)
SK1WG_FSTAB := $(LOCAL_PATH)/rootdir/etc/fstab.macallan.sdcard
else
SK1WG_FSTAB := $(LOCAL_PATH)/rootdir/etc/fstab.macallan
endif
PRODUCT_COPY_FILES += \
    $(SK1WG_FSTAB):$(TARGET_COPY_OUT_VENDOR)/etc/fstab.macallan \
    $(SK1WG_FSTAB):$(TARGET_COPY_OUT_RAMDISK)/fstab.macallan \
    $(LOCAL_PATH)/rootdir/etc/init.macallan.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.macallan.rc \
    $(LOCAL_PATH)/rootdir/etc/init.macallan.usb.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.macallan.usb.rc \
    $(LOCAL_PATH)/rootdir/etc/ueventd.macallan.rc:$(TARGET_COPY_OUT_VENDOR)/etc/ueventd.rc

# Graphics: NVIDIA's gralloc0 and HWC 1.x through the HIDL adapters, all
# passthrough (loaded in the client), as on html6405's Exynos 4 trees
PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator@2.0-impl-sk1wg \
    android.hardware.graphics.mapper@2.0-impl-sk1wg \
    android.hardware.graphics.composer@2.1-impl \
    libshim_atomic \
    libshim_graphicbuffer

# OpenGL ES 3.0 (Tegra 4 has no ES 3.1)
PRODUCT_VENDOR_PROPERTIES += \
    ro.opengles.version=196608

# Health (legacy 2.0 service with the default battery monitor: the CW2015
# fuel gauge is a standard power_supply)
PRODUCT_PACKAGES += \
    android.hardware.health@2.0-service

# Keymaster and gatekeeper (software; the Trusted Foundations keystore
# blob is not used)
PRODUCT_PACKAGES += \
    android.hardware.keymaster@3.0-impl \
    android.hardware.keymaster@3.0-service \
    android.hardware.gatekeeper@1.0-service.software

# Audio: stub HAL for the first boot (the TLV320AIC325x HAL comes later)
PRODUCT_PACKAGES += \
    android.hardware.audio@7.0-impl \
    android.hardware.audio.effect@7.0-impl \
    android.hardware.audio.service \
    audio.primary.default \
    audio.r_submix.default

# USB
PRODUCT_PACKAGES += \
    android.hardware.usb@1.0-service.basic

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.software.midi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.midi.xml

# Touchscreen and keyboard dock
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/idc/ft5x06_ts.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/ft5x06_ts.idc \
    $(LOCAL_PATH)/configs/keylayout/gpio-keys.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/gpio-keys.kl

# Space: the system partition (APP) is only 1000 MB. Apps are compiled on
# the device instead of at build time (only the boot image and
# system_server are preoptimised), and a few bundled apps are dropped.
PRODUCT_DEXPREOPT_SPEED_APPS :=
WITH_DEXPREOPT_BOOT_IMG_AND_SYSTEM_SERVER_ONLY := true
PRODUCT_PACKAGES += \
    RemovePackages

# Storage: no sdcardfs in this kernel
PRODUCT_PRODUCT_PROPERTIES += \
    external_storage.sdcardfs.enabled=false

# Debuggable builds: logs survive a crash
ifneq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_SYSTEM_PROPERTIES += \
    persist.logd.logpersistd=logcatd
endif

# Vendor blobs (extract-files.sh)
$(call inherit-product, vendor/jide/sk1wg/sk1wg-vendor.mk)

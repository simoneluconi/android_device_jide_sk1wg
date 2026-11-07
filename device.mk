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
    libshim_graphicbuffer \
    libcplconnectclient \
    libnvcpl

# Graphics properties. Tegra 4's GPU is OpenGL ES 2.0 only (stock declares
# 131072), but the version below is what html6405's Exynos 4 trees declare
# for their ES 2.0 Mali-400 on Android 14, which is known to work; revisit
# if an app picks an ES 3 path and fails.
PRODUCT_VENDOR_PROPERTIES += \
    ro.opengles.version=196610 \
    ro.sf.lcd_density=240 \
    ro.zygote.disable_gl_preload=1 \
    debug.hwui.renderer=opengl \
    debug.hwui.render_dirty_regions=false \
    debug.sf.enable_gl_backpressure=1 \
    ro.egl.destroy_after_detach=true

# SurfaceFlinger: the GLES RenderEngine. Skia's cannot import the 5.1 blobs'
# buffers as textures (primeShaderCache aborts with "Failed to create a
# valid texture"); html6405's smdk4412-common does the same.
PRODUCT_VENDOR_PROPERTIES += \
    debug.renderengine.backend=gles

# ART: 4 cores, but keep dex2oat's memory modest on first boot (apps are
# compiled on the device, see "Space" below)
PRODUCT_VENDOR_PROPERTIES += \
    dalvik.vm.dex2oat-Xmx=512m \
    dalvik.vm.dex2oat-threads=2 \
    dalvik.vm.boot-dex2oat-threads=4

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

# Audio: the TLV320AIC325x (ALSA card 1) through our tinyalsa HIDL 7.1
# HAL (audio/, from the emulator's), with stock Remix OS's routing.
# The policy and its volume tables: without them every stream had a
# volume range of 0..0 and the volume keys did nothing.
PRODUCT_PACKAGES += \
    android.hardware.audio@7.1-impl.sk1wg \
    android.hardware.audio.effect@7.0-impl \
    android.hardware.audio.service \
    audio.bluetooth.default \
    audio.r_submix.default \
    tinymix \
    tinyplay

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/audio/policy/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    $(LOCAL_PATH)/audio/policy/primary_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/primary_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/bluetooth_audio_policy_configuration_7_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth_audio_policy_configuration_7_0.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    frameworks/av/media/libeffects/data/audio_effects.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects.xml

# USB
PRODUCT_PACKAGES += \
    android.hardware.usb@1.0-service.basic

# adbd: the 3.4 f_fs has no AIO (io_submit on FunctionFS came in 3.15), so
# use adbd's legacy blocking USB code (as html6405's smdk4412-common). Without
# it the host sees the device but it stays "offline".
PRODUCT_VENDOR_PROPERTIES += \
    ro.adb.nonblocking_ffs=false

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.software.midi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.midi.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml

# Wi-Fi: no vendor Wi-Fi HAL. This bcmdhd (2016) has no nl80211 vendor
# commands (no wl_cfgvendor.c), so libwifi-hal-bcm fails "set hal start"
# and the HAL crashes; without a HAL the framework drives wlan0 through
# wificond and wpa_supplicant (optional since Android 11).
# wpa_supplicant/hostapd use the bcmdhd driver lib; the overlays carry
# stock Remix OS's settings.
PRODUCT_PACKAGES += \
    hostapd \
    libwpa_client \
    wificond \
    wpa_supplicant \
    wpa_supplicant.conf

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/wifi/wpa_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant_overlay.conf \
    $(LOCAL_PATH)/configs/wifi/p2p_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant_overlay.conf

PRODUCT_VENDOR_PROPERTIES += \
    wifi.interface=wlan0

# Bluetooth: HIDL 1.0 passthrough with AOSP's Broadcom libbt-vendor
# (configs/bluetooth/vnd_sk1wg.txt)
PRODUCT_PACKAGES += \
    android.hardware.bluetooth@1.0-impl \
    android.hardware.bluetooth@1.0-service \
    libbt-vendor

# The BT address: the chip has none programmed and stock kept one in
# /data. init.sk1wg.bdaddr.sh makes a random locally administered one
# on the first boot and keeps it.
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/bin/init.sk1wg.bdaddr.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.sk1wg.bdaddr.sh

PRODUCT_VENDOR_PROPERTIES += \
    ro.bt.bdaddr_path=/data/vendor/bluetooth/bdaddr

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

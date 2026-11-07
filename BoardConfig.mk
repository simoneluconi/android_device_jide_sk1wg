#
# SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
# SPDX-License-Identifier: Apache-2.0
#
# Board configuration for the Jide Remix Ultra Tablet (REMIX SK1WG):
# NVIDIA Tegra 4 (T114, 4x Cortex-A15, ARMv7), "macallan" reference board,
# NVIDIA 3.4 kernel, stock partitions (never repartitioned, see CLAUDE.md).
#

DEVICE_PATH := device/jide/sk1wg

# Architecture
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := cortex-a15
TARGET_CPU_SMP := true

# Board
TARGET_BOARD_PLATFORM := tegra
TARGET_BOOTLOADER_BOARD_NAME := macallan
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true

# Legacy device on LineageOS-UL (as html6405's smdk4412-common does)
BUILD_BROKEN_DUP_RULES := true
BUILD_BROKEN_MISSING_REQUIRED_MODULES := true
BUILD_BROKEN_ELF_PREBUILT_PRODUCT_COPY_FILES := true
BUILD_BROKEN_VINTF_PRODUCT_COPY_FILES := true
BUILD_BROKEN_USES_BUILD_COPY_HEADERS := true
RELAX_USES_LIBRARY_CHECK := true
TARGET_OTA_ALLOW_NON_AB := true
PRODUCT_ENFORCE_VINTF_MANIFEST_OVERRIDE := true
TARGET_HAS_MEMFD_BACKPORT := true

# Kernel: our 3.4 tree, built with GCC 4.9. The bootloader passes its own
# command line; the kernel appends androidboot.hardware=macallan (and, for
# the bring-up, SELinux permissive) with CONFIG_CMDLINE_EXTEND, because it
# is not known yet whether the bootloader appends the boot image's.
BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_IMAGE_NAME := zImage
# loglevel=4: the bootloader adds console=ttyS0,115200n8, and every kernel
# message at a lower level would be written synchronously to that UART.
# oops=panic (bring-up): an Oops panics, and the kernel's
# TEGRA_PANIC_TO_BOOTLOADER then restarts into fastboot with last_kmsg kept.
BOARD_KERNEL_CMDLINE := androidboot.hardware=macallan androidboot.selinux=permissive loglevel=4 oops=panic
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x01000000 --tags_offset 0x00000100 --second_offset 0x00f00000
TARGET_KERNEL_SOURCE := kernel/jide/sk1wg
TARGET_KERNEL_CONFIG := lineageos_sk1wg_defconfig
# The few =m modules (none needed) go to /system/lib/modules: with vendor
# inside system, kernel.mk's vendor install lists the wrong paths
NEED_KERNEL_MODULE_SYSTEM := true
TARGET_KERNEL_CLANG_COMPILE := false
TARGET_KERNEL_LLVM_BINUTILS := false
KERNEL_TOOLCHAIN := $(realpath $(TOP))/prebuilts/gcc/$(HOST_OS)-x86/arm/arm-linux-androideabi-4.9/bin
TARGET_KERNEL_CROSS_COMPILE_PREFIX := arm-linux-androidkernel-
# Host tools of a 3.4 tree need -fcommon (old dtc); the build sandbox
# would make /proc/version say nobody@android-build
TARGET_KERNEL_ADDITIONAL_FLAGS := \
    HOSTCFLAGS="-fuse-ld=lld -Wno-unused-command-line-argument -fcommon" \
    KBUILD_BUILD_USER=simoneluconi \
    KBUILD_BUILD_HOST=jideremixtabletrevived

# Partitions (stock GPT, sizes from "fastboot getvar all"):
#   LNX  boot          8 MB
#   APP  system     1000 MB (with /system/vendor: the vendor files are ~35 MB)
#   CAC  system_ext  768 MB (stock /cache, unused by LineageOS here)
#   UDA  data       ~56 GB
# system with system_ext does not fit in APP (1210 MB), so system_ext gets
# the 768 MB CAC and vendor, too small to need a partition, lives in
# system. SOS (recovery) keeps the stock Remix OS recovery; there is no
# room for the LineageOS one.
BOARD_BOOTIMAGE_PARTITION_SIZE := 8388608
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1048576000
BOARD_SYSTEM_EXTIMAGE_PARTITION_SIZE := 805306368
BOARD_SYSTEM_EXTIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_USES_SYSTEM_EXTIMAGE := true
TARGET_COPY_OUT_SYSTEM_EXT := system_ext
TARGET_COPY_OUT_VENDOR := system/vendor
BOARD_USERDATAIMAGE_PARTITION_SIZE := 60578332672
BOARD_FLASH_BLOCK_SIZE := 4096
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_PROPERTY_OVERRIDES_SPLIT_ENABLED := true
TARGET_NO_RECOVERY := true
BOARD_ROOT_EXTRA_FOLDERS :=
BOARD_ROOT_EXTRA_SYMLINKS :=

# Wi-Fi: Broadcom BCM43341 (USI WM-BAN-BM-13) on SDIO, bcmdhd built into
# the kernel as module "bcm43341". Firmware and NVRAM are extracted to
# /vendor (the kernel config has the same default paths).
BOARD_WLAN_DEVICE := bcmdhd
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_bcmdhd
WIFI_DRIVER_FW_PATH_PARAM := "/sys/module/bcm43341/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA := "/vendor/firmware/bcm43341/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_AP := "/vendor/firmware/bcm43341/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_P2P := "/vendor/firmware/bcm43341/fw_bcmdhd_p2p.bin"
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true

# Bluetooth: the BCM43341's BT core on /dev/ttyHS2, powered through the
# "bluedroid_pm" rfkill; AOSP's Broadcom vendor library, as stock
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true
BOARD_CUSTOM_BT_CONFIG := $(DEVICE_PATH)/configs/bluetooth/vnd_sk1wg.txt

# Graphics: NVIDIA's T114 gralloc0 and HWC 1.x
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
TARGET_USES_HWC2 := true

# Shims for the Android 5.1 graphics blobs (see shims/). The linker
# matches the library's realpath, which is under /system/vendor (/vendor is
# a symlink); the /vendor entries are kept in case it does not resolve.
TARGET_LD_SHIM_LIBS := \
    /system/vendor/lib/hw/gralloc.tegra.so|/vendor/lib/libshim_atomic.so \
    /system/vendor/lib/hw/hwcomposer.tegra.so|/vendor/lib/libshim_atomic.so \
    /system/vendor/lib/hw/hwcomposer.tegra.so|/vendor/lib/libshim_graphicbuffer.so \
    /vendor/lib/hw/gralloc.tegra.so|/vendor/lib/libshim_atomic.so \
    /vendor/lib/hw/hwcomposer.tegra.so|/vendor/lib/libshim_atomic.so \
    /vendor/lib/hw/hwcomposer.tegra.so|/vendor/lib/libshim_graphicbuffer.so

# VINTF
DEVICE_MANIFEST_FILE := $(DEVICE_PATH)/manifest.xml
DEVICE_MATRIX_FILE := $(DEVICE_PATH)/compatibility_matrix.xml
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE += $(DEVICE_PATH)/framework_compatibility_matrix.xml

# SELinux: permissive for the bring-up (see BOARD_KERNEL_CMDLINE), with a
# minimal vendor policy for the NVIDIA device nodes
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/vendor
# The Treble compat tests (prebuilts/api/30.0-34.0, built as "user") trip
# over the LineageOS su domain's rules; html6405's smdk4412-common, which
# the Galaxy Camera builds with, sets the same
SELINUX_IGNORE_NEVERALLOWS := true

# Inherit the proprietary files
include vendor/jide/sk1wg/BoardConfigVendor.mk

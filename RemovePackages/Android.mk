#
# SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
# SPDX-License-Identifier: Apache-2.0
#
# Drop the apps a Wi-Fi tablet without a camera HAL does not need, and the
# theme extras: the system partition is only 1000 MB. Overriding them keeps
# LineageOS itself untouched. Kept: browser, gallery, clock, calculator,
# files, music, Aurora Store, the wallpaper and theme picker.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := RemovePackages
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_TAGS := optional
LOCAL_OVERRIDES_PACKAGES := \
    Aperture \
    BasicDreams \
    BuiltInPrintService \
    CallLogBackup \
    Camera2 \
    CameraExtensionsProxy \
    CellBroadcastLegacyApp \
    DeviceAsWebcam \
    Dialer \
    EasterEgg \
    EmergencyInfo \
    IconPackCircularAndroidOverlay \
    IconPackCircularLauncherOverlay \
    IconPackCircularSettingsOverlay \
    IconPackCircularSystemUIOverlay \
    IconPackFilledAndroidOverlay \
    IconPackFilledLauncherOverlay \
    IconPackFilledSettingsOverlay \
    IconPackFilledSystemUIOverlay \
    IconPackKaiAndroidOverlay \
    IconPackKaiLauncherOverlay \
    IconPackKaiSettingsOverlay \
    IconPackKaiSystemUIOverlay \
    IconPackRoundedAndroidOverlay \
    IconPackRoundedLauncherOverlay \
    IconPackRoundedSettingsOverlay \
    IconPackRoundedSystemUIOverlay \
    IconPackSamAndroidOverlay \
    IconPackSamLauncherOverlay \
    IconPackSamSettingsOverlay \
    IconPackSamSystemUIOverlay \
    IconPackVictorAndroidOverlay \
    IconPackVictorLauncherOverlay \
    IconPackVictorSettingsOverlay \
    IconPackVictorSystemUIOverlay \
    ImsServiceEntitlement \
    NfcNci \
    ONS \
    OpenCamera \
    PhotoTable \
    QuickAccessWallet \
    Recorder \
    SecureElement \
    Stk \
    Updater \
    WAPPushManager
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_SRC_FILES := /dev/null
include $(BUILD_PREBUILT)

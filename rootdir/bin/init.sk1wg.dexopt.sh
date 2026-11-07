#!/vendor/bin/sh
#
# SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
# SPDX-License-Identifier: Apache-2.0
#
# The system partition is too small for build-time app compilation
# (device.mk: WITH_DEXPREOPT_BOOT_IMG_AND_SYSTEM_SERVER_ONLY), so apps only
# have "verify" until the idle job compiles them, and JIT-only UI code is
# slow on the T114 (Settings scroll: 90th percentile frame 900 ms with
# verify, 48 ms with speed). Compile the apps used all the time with
# "speed" into /data once after boot; already compiled ones are skipped.

for p in \
    com.android.systemui \
    com.android.launcher3 \
    com.android.settings \
    com.android.inputmethod.latin \
    com.android.webview \
    org.lineageos.jelly \
    com.android.documentsui \
    com.android.permissioncontroller \
    com.android.deskclock; do
    cmd package compile -m speed "$p" > /dev/null 2>&1
done

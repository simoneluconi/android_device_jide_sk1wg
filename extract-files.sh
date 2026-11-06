#!/bin/bash
#
# SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
# SPDX-License-Identifier: Apache-2.0
#
# Usage: extract-files.sh <dir holding the stock 2.0.305 system/>
# (see proprietary-files.txt for how to make it)
#

set -e

DEVICE=sk1wg
VENDOR=jide

MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

export TARGET_DISABLE_XML_FIXING=true

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

function blob_fixup() {
    case "${1}" in
        # The EGL/GLES wrappers dlopen their implementation and libEGL from
        # /system/lib; on LineageOS they live in /vendor/lib (same length)
        vendor/lib/egl/libEGL_tegra.so | vendor/lib/egl/libEGL_tegra_impl.so | \
        vendor/lib/egl/libGLESv1_CM_tegra.so | vendor/lib/egl/libGLESv2_tegra.so)
            sed -i "s|/system/lib/|/vendor/lib/|g" "${2}"
            ;;
    esac
}

SRC="${1:-adb}"

setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false true

extract "${MY_DIR}/proprietary-files.txt" "${SRC}"

"${MY_DIR}/setup-makefiles.sh"

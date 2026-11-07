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
        # create_graphic_buffer() allocates the GraphicBuffer itself with
        # operator new(128), the Android 5.1 size; Android 14's is 160
        # (same layout up to there: RefBase at 0, the native buffer at 8,
        # its handle at 60). Patch "movs r0, #0x80" to "movs r0, #0xa0".
        # shims/graphicbuffer.cpp static_asserts the size.
        vendor/lib/hw/hwcomposer.tegra.so)
            if [ "$(xxd -p -s 0xc11c -l 2 "${2}")" = "8020" ]; then
                printf '\xa0' | dd of="${2}" bs=1 seek=$((0xc11c)) conv=notrunc status=none
            fi
            [ "$(xxd -p -s 0xc11c -l 2 "${2}")" = "a020" ] || {
                echo "hwcomposer.tegra.so: unexpected bytes at 0xc11c" >&2
                return 1
            }
            ;;
    esac
}

SRC="${1:-adb}"

setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false true

extract "${MY_DIR}/proprietary-files.txt" "${SRC}"

"${MY_DIR}/setup-makefiles.sh"

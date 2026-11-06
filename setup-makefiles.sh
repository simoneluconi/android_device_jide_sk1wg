#!/bin/bash
#
# SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
# SPDX-License-Identifier: Apache-2.0
#

set -e

DEVICE=sk1wg
VENDOR=jide

MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}"

write_headers

write_makefiles "${MY_DIR}/proprietary-files.txt"

# The EGL loader picks the "tegra" driver
cat << EOF >> "${PRODUCTMK}"

PRODUCT_VENDOR_PROPERTIES += \\
    ro.hardware.egl=tegra \\
    ro.hardware.gralloc=tegra \\
    ro.hardware.hwcomposer=tegra
EOF

write_footers

/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define GLES2_SHIM_TAG "libGLESv2_tegra_shim"

/* Full paths: a bare name is looked up in /vendor/lib, not in egl/. */
#define NV_GLES2_LIB "/vendor/lib/egl/libGLESv2_tegra_nv.so"
#define NV_EGL_LIB "/vendor/lib/egl/libEGL_tegra_nv.so"

/* dlsym() in NVIDIA's GLES 2 library, loaded on first use; NULL if missing. */
__attribute__((visibility("hidden"))) void *gles2_tegra_nv_sym(const char *name);

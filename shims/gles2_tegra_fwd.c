/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * libGLESv2_tegra.so: a pass-through in front of NVIDIA's GLES 2 library
 * (shipped as libGLESv2_tegra_nv.so), for one driver quirk; see
 * gles2_tegra_copy.c. Android's EGL loader dlsym()s every core GL entry
 * point from this library, so it has to export all of them.
 *
 * No GL headers here: the forwarders below are declared with generic
 * arguments. armeabi-v7a uses the softfp AAPCS, so GL's float arguments
 * travel in core registers like everything else: the first four words in
 * r0-r3, the rest on the stack, which a forwarder with eight extra words
 * re-reads from the caller's frame (GLES 2's widest call, glTexImage2D and
 * glTexSubImage2D, takes nine). A 64-bit return covers r0 and r1.
 */

#include <android/log.h>
#include <dlfcn.h>
#include <pthread.h>

#include "gles2_tegra.h"

static void *nv_gles2;
static pthread_once_t nv_once = PTHREAD_ONCE_INIT;

static void nv_load(void)
{
    nv_gles2 = dlopen(NV_GLES2_LIB, RTLD_NOW | RTLD_LOCAL);
    if (!nv_gles2)
        __android_log_print(ANDROID_LOG_ERROR, GLES2_SHIM_TAG, "cannot load %s: %s",
                            NV_GLES2_LIB, dlerror());
}

void *gles2_tegra_nv_sym(const char *name)
{
    pthread_once(&nv_once, nv_load);
    return nv_gles2 ? dlsym(nv_gles2, name) : NULL;
}

typedef long long (*nv_any_fn)(void *, void *, void *, void *, void *, void *, void *, void *,
                               void *, void *, void *, void *);

/* If NVIDIA's library or a symbol is missing, return 0; nv_load() logged why. */
#define GLES2_FUNC(name)                                                             \
    long long name(void *a, void *b, void *c, void *d, void *e, void *f, void *g,   \
                   void *h, void *i, void *j, void *k, void *l)                     \
    {                                                                                \
        static nv_any_fn real;                                                       \
        if (!real)                                                                   \
            real = (nv_any_fn)gles2_tegra_nv_sym(#name);                             \
        if (!real)                                                                   \
            return 0;                                                                \
        return real(a, b, c, d, e, f, g, h, i, j, k, l);                             \
    }

#include "gles2_tegra_names.h"

/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * libEGL_tegra.so: a pass-through in front of NVIDIA's EGL loader (shipped
 * as libEGL_tegra_nv.so), for one driver quirk.
 *
 * The T114 EGL rejects eglCreateImageKHR(EGL_GL_TEXTURE_2D_KHR) with
 * EGL_BAD_PARAMETER when the texture is not mipmap complete, e.g. only level
 * 0 defined while GL_TEXTURE_MIN_FILTER is still the default
 * GL_NEAREST_MIPMAP_LINEAR. The EGL_KHR_gl_image spec only requires level 0.
 * ANGLE's GL backend creates its native textures that way (it applies
 * sampler state at draw time), so every EGLImage that Chromium's
 * EGLImageBacking asks for failed and WebView drew nothing.
 *
 * On such a failure, for a texture of the current context, retry with a
 * non-mipmap minification filter and restore the filter afterwards.
 * Everything else goes straight to NVIDIA's loader.
 */

#define LOG_TAG "libEGL_tegra_shim"

#include <dlfcn.h>
#include <pthread.h>
#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <log/log.h>

/* Full paths: a bare name is looked up in /vendor/lib, not in egl/. */
#define NV_EGL_LIB "/vendor/lib/egl/libEGL_tegra_nv.so"
#define NV_GLES2_LIB "/vendor/lib/egl/libGLESv2_tegra.so"

static void *nv_egl;
static pthread_once_t nv_once = PTHREAD_ONCE_INIT;

static void nv_load(void)
{
    nv_egl = dlopen(NV_EGL_LIB, RTLD_NOW | RTLD_LOCAL);
    if (!nv_egl)
        ALOGE("cannot load %s: %s", NV_EGL_LIB, dlerror());
}

static void *nv_sym(const char *name)
{
    pthread_once(&nv_once, nv_load);
    return nv_egl ? dlsym(nv_egl, name) : NULL;
}

/* One forwarder per function NVIDIA's loader exports. */
#define FWD(ret, name, params, args)                                  \
    ret name params                                                   \
    {                                                                 \
        static ret(*real) params;                                     \
        if (!real)                                                    \
            real = (ret(*) params)nv_sym(#name);                      \
        return real args;                                             \
    }

FWD(EGLDisplay, eglGetDisplay, (EGLNativeDisplayType d), (d))
FWD(EGLBoolean, eglInitialize, (EGLDisplay d, EGLint *ma, EGLint *mi), (d, ma, mi))
FWD(EGLBoolean, eglTerminate, (EGLDisplay d), (d))
FWD(const char *, eglQueryString, (EGLDisplay d, EGLint n), (d, n))
FWD(EGLBoolean, eglGetConfigs, (EGLDisplay d, EGLConfig *c, EGLint s, EGLint *n), (d, c, s, n))
FWD(EGLBoolean, eglChooseConfig,
    (EGLDisplay d, const EGLint *a, EGLConfig *c, EGLint s, EGLint *n), (d, a, c, s, n))
FWD(EGLBoolean, eglGetConfigAttrib, (EGLDisplay d, EGLConfig c, EGLint a, EGLint *v),
    (d, c, a, v))
FWD(EGLSurface, eglCreateWindowSurface,
    (EGLDisplay d, EGLConfig c, EGLNativeWindowType w, const EGLint *a), (d, c, w, a))
FWD(EGLSurface, eglCreatePbufferSurface, (EGLDisplay d, EGLConfig c, const EGLint *a),
    (d, c, a))
FWD(EGLSurface, eglCreatePixmapSurface,
    (EGLDisplay d, EGLConfig c, EGLNativePixmapType p, const EGLint *a), (d, c, p, a))
FWD(EGLBoolean, eglDestroySurface, (EGLDisplay d, EGLSurface s), (d, s))
FWD(EGLBoolean, eglQuerySurface, (EGLDisplay d, EGLSurface s, EGLint a, EGLint *v),
    (d, s, a, v))
FWD(EGLBoolean, eglBindAPI, (EGLenum api), (api))
FWD(EGLenum, eglQueryAPI, (void), ())
FWD(EGLBoolean, eglWaitClient, (void), ())
FWD(EGLBoolean, eglReleaseThread, (void), ())
FWD(EGLSurface, eglCreatePbufferFromClientBuffer,
    (EGLDisplay d, EGLenum t, EGLClientBuffer b, EGLConfig c, const EGLint *a),
    (d, t, b, c, a))
FWD(EGLBoolean, eglSurfaceAttrib, (EGLDisplay d, EGLSurface s, EGLint a, EGLint v),
    (d, s, a, v))
FWD(EGLBoolean, eglBindTexImage, (EGLDisplay d, EGLSurface s, EGLint b), (d, s, b))
FWD(EGLBoolean, eglReleaseTexImage, (EGLDisplay d, EGLSurface s, EGLint b), (d, s, b))
FWD(EGLBoolean, eglSwapInterval, (EGLDisplay d, EGLint i), (d, i))
FWD(EGLContext, eglCreateContext,
    (EGLDisplay d, EGLConfig c, EGLContext s, const EGLint *a), (d, c, s, a))
FWD(EGLBoolean, eglDestroyContext, (EGLDisplay d, EGLContext c), (d, c))
FWD(EGLBoolean, eglMakeCurrent, (EGLDisplay d, EGLSurface w, EGLSurface r, EGLContext c),
    (d, w, r, c))
FWD(EGLContext, eglGetCurrentContext, (void), ())
FWD(EGLSurface, eglGetCurrentSurface, (EGLint r), (r))
FWD(EGLDisplay, eglGetCurrentDisplay, (void), ())
FWD(EGLBoolean, eglQueryContext, (EGLDisplay d, EGLContext c, EGLint a, EGLint *v),
    (d, c, a, v))
FWD(EGLint, eglGetError, (void), ())
FWD(EGLBoolean, eglWaitGL, (void), ())
FWD(EGLBoolean, eglWaitNative, (EGLint e), (e))
FWD(EGLBoolean, eglSwapBuffers, (EGLDisplay d, EGLSurface s), (d, s))
FWD(EGLBoolean, eglCopyBuffers, (EGLDisplay d, EGLSurface s, EGLNativePixmapType p),
    (d, s, p))

/*
 * NVIDIA's private loader interface: libGLESv2_tegra.so and
 * libGLESv1_CM_tegra.so open /vendor/lib/egl/libEGL_tegra.so by path and
 * call these to get their dispatch tables. Their prototypes are not
 * public, so forward any arguments: with the softfp AAPCS, the first four
 * words travel in r0-r3 and the next four are re-read from the caller's
 * stack, and a 64-bit return covers r0 and r1.
 */
typedef long long (*nv_any_fn)(void *, void *, void *, void *, void *, void *, void *, void *);

#define FWD_ANY(name)                                                      \
    long long name(void *a, void *b, void *c, void *d, void *e, void *f,   \
                   void *g, void *h)                                       \
    {                                                                      \
        static nv_any_fn real;                                             \
        if (!real)                                                         \
            real = (nv_any_fn)nv_sym(#name);                               \
        return real(a, b, c, d, e, f, g, h);                               \
    }

FWD_ANY(NvEglInitClientFunctions)
FWD_ANY(NvEglRegClientApi)

/* The GL entry points the retry needs, from NVIDIA's GLES loader. */
static struct {
    void (*GetIntegerv)(GLenum, GLint *);
    void (*BindTexture)(GLenum, GLuint);
    void (*GetTexParameteriv)(GLenum, GLenum, GLint *);
    void (*TexParameteri)(GLenum, GLenum, GLint);
} gl;
static pthread_once_t gl_once = PTHREAD_ONCE_INIT;

static void gl_load(void)
{
    void *h = dlopen(NV_GLES2_LIB, RTLD_NOW | RTLD_LOCAL);

    if (!h) {
        ALOGE("cannot load %s: %s", NV_GLES2_LIB, dlerror());
        return;
    }
    gl.GetIntegerv = dlsym(h, "glGetIntegerv");
    gl.BindTexture = dlsym(h, "glBindTexture");
    gl.GetTexParameteriv = dlsym(h, "glGetTexParameteriv");
    gl.TexParameteri = dlsym(h, "glTexParameteri");
}

static int is_mipmap_filter(GLint f)
{
    return f == GL_NEAREST_MIPMAP_NEAREST || f == GL_LINEAR_MIPMAP_NEAREST ||
           f == GL_NEAREST_MIPMAP_LINEAR || f == GL_LINEAR_MIPMAP_LINEAR;
}

static PFNEGLCREATEIMAGEKHRPROC real_create_image;

static EGLImageKHR shim_eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                                          EGLClientBuffer buffer, const EGLint *attrs)
{
    EGLImageKHR img = real_create_image(dpy, ctx, target, buffer, attrs);
    GLint old_binding = 0, old_min = 0;
    GLuint tex = (GLuint)(uintptr_t)buffer;

    if (img != EGL_NO_IMAGE_KHR || target != EGL_GL_TEXTURE_2D_KHR || !tex ||
        ctx != eglGetCurrentContext())
        return img;

    pthread_once(&gl_once, gl_load);
    if (!gl.GetIntegerv || !gl.BindTexture || !gl.GetTexParameteriv || !gl.TexParameteri)
        return img;

    gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &old_binding);
    gl.BindTexture(GL_TEXTURE_2D, tex);
    gl.GetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &old_min);
    if (is_mipmap_filter(old_min)) {
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                         old_min == GL_NEAREST_MIPMAP_NEAREST ? GL_NEAREST : GL_LINEAR);
        img = real_create_image(dpy, ctx, target, buffer, attrs);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, old_min);
        ALOGV("texture %u: retry with a non-mipmap filter %s", tex,
              img != EGL_NO_IMAGE_KHR ? "worked" : "failed too");
    }
    gl.BindTexture(GL_TEXTURE_2D, (GLuint)old_binding);
    return img;
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *name)
{
    static __eglMustCastToProperFunctionPointerType (*real)(const char *);
    __eglMustCastToProperFunctionPointerType p;

    if (!real)
        real = (__eglMustCastToProperFunctionPointerType(*)(const char *))nv_sym(
                "eglGetProcAddress");
    if (!real)
        return NULL;
    p = real(name);
    if (p && name && !strcmp(name, "eglCreateImageKHR")) {
        real_create_image = (PFNEGLCREATEIMAGEKHRPROC)p;
        return (__eglMustCastToProperFunctionPointerType)shim_eglCreateImageKHR;
    }
    return p;
}

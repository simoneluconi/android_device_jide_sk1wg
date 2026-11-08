/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * glCopyTexSubImage2D() on the GPU.
 *
 * The T114 driver copies the framebuffer into the texture on the CPU: it
 * maps both surfaces and converts the pixels in a loop, with a page fault
 * per page of the mapping and cache maintenance on every CPU. Chromium
 * (through ANGLE) copies the backdrop of every CSS backdrop-filter
 * element this way, every frame, so a page with four blurred cards
 * rendered at about two frames per second.
 *
 * The driver has GL_NV_framebuffer_blit, which runs on the GPU. Attach the
 * destination texture to a scratch framebuffer bound as the draw
 * framebuffer and blit the same rectangle from the current read
 * framebuffer: the result is the same as the copy (no scaling, no
 * filtering). Everything the blit changes is restored, so callers that
 * track GL state (ANGLE, Skia) see nothing. When the blit cannot do it
 * (no extension, a destination format that is not color-renderable, a
 * pending GL error, anything else), fall back to the driver's copy.
 *
 * debug.sk1wg.copy_blit=0 turns the blit off, debug.sk1wg.copy_blit=log
 * logs every call (both read when a process first copies).
 */

#include <android/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <sys/system_properties.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "gles2_tegra.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, GLES2_SHIM_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, GLES2_SHIM_TAG, __VA_ARGS__)

/* GL_NV_framebuffer_blit */
#ifndef GL_READ_FRAMEBUFFER_NV
#define GL_READ_FRAMEBUFFER_NV 0x8CA8
#define GL_DRAW_FRAMEBUFFER_NV 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_NV 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_NV 0x8CAA
#endif

typedef void (*blit_fn)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield,
                        GLenum);

enum { MODE_OFF, MODE_ON, MODE_LOG };

static struct {
    void (*CopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    GLenum (*GetError)(void);
    void (*GetIntegerv)(GLenum, GLint *);
    GLboolean (*IsEnabled)(GLenum);
    void (*Enable)(GLenum);
    void (*Disable)(GLenum);
    void (*GenFramebuffers)(GLsizei, GLuint *);
    void (*DeleteFramebuffers)(GLsizei, const GLuint *);
    void (*BindFramebuffer)(GLenum, GLuint);
    void (*FramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
    GLenum (*CheckFramebufferStatus)(GLenum);
    blit_fn BlitFramebufferNV;
} gl;
static int mode;
static pthread_once_t gl_once = PTHREAD_ONCE_INIT;

static void gl_load(void)
{
    char prop[PROP_VALUE_MAX] = "";
    __eglMustCastToProperFunctionPointerType (*get_proc)(const char *) = NULL;
    void *egl;

    gl.CopyTexSubImage2D = gles2_tegra_nv_sym("glCopyTexSubImage2D");
    gl.GetError = gles2_tegra_nv_sym("glGetError");
    gl.GetIntegerv = gles2_tegra_nv_sym("glGetIntegerv");
    gl.IsEnabled = gles2_tegra_nv_sym("glIsEnabled");
    gl.Enable = gles2_tegra_nv_sym("glEnable");
    gl.Disable = gles2_tegra_nv_sym("glDisable");
    gl.GenFramebuffers = gles2_tegra_nv_sym("glGenFramebuffers");
    gl.DeleteFramebuffers = gles2_tegra_nv_sym("glDeleteFramebuffers");
    gl.BindFramebuffer = gles2_tegra_nv_sym("glBindFramebuffer");
    gl.FramebufferTexture2D = gles2_tegra_nv_sym("glFramebufferTexture2D");
    gl.CheckFramebufferStatus = gles2_tegra_nv_sym("glCheckFramebufferStatus");

    /* NVIDIA's own loader: extensions are not exported by the GLES library. */
    egl = dlopen(NV_EGL_LIB, RTLD_NOW | RTLD_LOCAL);
    if (egl)
        get_proc = dlsym(egl, "eglGetProcAddress");
    if (get_proc)
        gl.BlitFramebufferNV = (blit_fn)get_proc("glBlitFramebufferNV");

    __system_property_get("debug.sk1wg.copy_blit", prop);
    mode = !strcmp(prop, "0") ? MODE_OFF : !strcmp(prop, "log") ? MODE_LOG : MODE_ON;

    if (!gl.GetError || !gl.GetIntegerv || !gl.IsEnabled || !gl.Enable || !gl.Disable ||
        !gl.GenFramebuffers || !gl.DeleteFramebuffers || !gl.BindFramebuffer ||
        !gl.FramebufferTexture2D || !gl.CheckFramebufferStatus || !gl.BlitFramebufferNV) {
        if (mode != MODE_OFF)
            LOGE("GL entry points missing (blit %p): copies stay on the CPU",
                 gl.BlitFramebufferNV);
        mode = MODE_OFF;
    }
}

/* 0 when the blit did the copy, otherwise why not (for the log). */
static const char *copy_by_blit(GLint level, GLint xoff, GLint yoff, GLint x, GLint y,
                                GLsizei w, GLsizei h)
{
    GLint tex = 0, draw_fb = 0;
    GLuint fb = 0;
    GLboolean scissor;
    GLenum status, err;

    /* The error we would read after the blit must be the blit's. */
    if (gl.GetError() != GL_NO_ERROR)
        return "error pending";
    gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    if (!tex)
        return "no texture bound";
    gl.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_NV, &draw_fb);

    gl.GenFramebuffers(1, &fb);
    gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER_NV, fb);
    gl.FramebufferTexture2D(GL_DRAW_FRAMEBUFFER_NV, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            (GLuint)tex, level);
    status = gl.CheckFramebufferStatus(GL_DRAW_FRAMEBUFFER_NV);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        /* glCopyTexSubImage2D ignores the scissor, a blit does not */
        scissor = gl.IsEnabled(GL_SCISSOR_TEST);
        if (scissor)
            gl.Disable(GL_SCISSOR_TEST);
        gl.BlitFramebufferNV(x, y, x + w, y + h, xoff, yoff, xoff + w, yoff + h,
                             GL_COLOR_BUFFER_BIT, GL_NEAREST);
        if (scissor)
            gl.Enable(GL_SCISSOR_TEST);
    }
    err = gl.GetError();

    /* The draw binding is the only one changed; the read one is the source. */
    gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER_NV, (GLuint)draw_fb);
    gl.DeleteFramebuffers(1, &fb);

    if (status != GL_FRAMEBUFFER_COMPLETE)
        return "texture not color-renderable";
    if (err != GL_NO_ERROR)
        return "blit failed";
    return NULL;
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y,
                         GLsizei w, GLsizei h)
{
    const char *why = "off";

    pthread_once(&gl_once, gl_load);
    if (mode != MODE_OFF) {
        if (target != GL_TEXTURE_2D)
            why = "not a 2D texture";
        else if (w <= 0 || h <= 0)
            why = "empty";
        else
            why = copy_by_blit(level, xoff, yoff, x, y, w, h);
        if (mode == MODE_LOG)
            LOGI("copy level %d %dx%d from (%d,%d) to (%d,%d): %s", level, w, h, x, y, xoff,
                 yoff, why ? why : "blit");
        if (!why)
            return;
    }
    if (gl.CopyTexSubImage2D)
        gl.CopyTexSubImage2D(target, level, xoff, yoff, x, y, w, h);
}

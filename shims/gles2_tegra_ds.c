/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * GL_OES_packed_depth_stencil on top of separate depth and stencil buffers.
 *
 * Chromium refuses WebGL without OES_packed_depth_stencil ("OES_packed_
 * depth_stencil support is required."): the WebGL default framebuffer is a
 * DEPTH24_STENCIL8 renderbuffer. The T114 driver has no packed format
 * (GL_INVALID_ENUM for GL_DEPTH24_STENCIL8_OES, also multisampled), but a
 * framebuffer with a GL_DEPTH_COMPONENT24_OES renderbuffer on the depth
 * attachment and a GL_STENCIL_INDEX8 one on the stencil attachment is
 * complete and both tests work, also with GL_NV_framebuffer_multisample.
 *
 * So the extension is advertised, and a "packed" renderbuffer is its depth
 * half: storage with GL_DEPTH24_STENCIL8_OES gives the bound renderbuffer
 * depth-24 storage and a hidden STENCIL_INDEX8 companion of the same size
 * and samples. Attaching the renderbuffer to GL_STENCIL_ATTACHMENT attaches
 * the companion instead; queries map back; deleting it deletes both.
 *
 * Renderbuffer names are per share group and a process has several (hwui,
 * ANGLE, ...), so a companion is only used after checking, in the current
 * context, that the renderbuffer is still depth-24 storage of the recorded
 * size and the companion is still a stencil buffer of that size.
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "gles2_tegra.h"

#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#endif
#define GL_RENDERBUFFER_SAMPLES_NV 0x8CAB

#define PACKED_EXT "GL_OES_packed_depth_stencil"

typedef void (*storage_ms_fn)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);

static struct {
    const GLubyte *(*GetString)(GLenum);
    void (*RenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei);
    void (*FramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint);
    void (*DeleteRenderbuffers)(GLsizei, const GLuint *);
    void (*GetRenderbufferParameteriv)(GLenum, GLenum, GLint *);
    void (*GetFramebufferAttachmentParameteriv)(GLenum, GLenum, GLenum, GLint *);
    void (*GenRenderbuffers)(GLsizei, GLuint *);
    void (*BindRenderbuffer)(GLenum, GLuint);
    GLboolean (*IsRenderbuffer)(GLuint);
    void (*GetIntegerv)(GLenum, GLint *);
} gl;
static storage_ms_fn real_storage_ms;
static pthread_once_t gl_once = PTHREAD_ONCE_INIT;

static void gl_load(void)
{
    gl.GetString = gles2_tegra_nv_sym("glGetString");
    gl.RenderbufferStorage = gles2_tegra_nv_sym("glRenderbufferStorage");
    gl.FramebufferRenderbuffer = gles2_tegra_nv_sym("glFramebufferRenderbuffer");
    gl.DeleteRenderbuffers = gles2_tegra_nv_sym("glDeleteRenderbuffers");
    gl.GetRenderbufferParameteriv = gles2_tegra_nv_sym("glGetRenderbufferParameteriv");
    gl.GetFramebufferAttachmentParameteriv =
            gles2_tegra_nv_sym("glGetFramebufferAttachmentParameteriv");
    gl.GenRenderbuffers = gles2_tegra_nv_sym("glGenRenderbuffers");
    gl.BindRenderbuffer = gles2_tegra_nv_sym("glBindRenderbuffer");
    gl.IsRenderbuffer = gles2_tegra_nv_sym("glIsRenderbuffer");
    gl.GetIntegerv = gles2_tegra_nv_sym("glGetIntegerv");
}

static int gl_ok(void)
{
    pthread_once(&gl_once, gl_load);
    return gl.GetString && gl.RenderbufferStorage && gl.FramebufferRenderbuffer &&
           gl.DeleteRenderbuffers && gl.GetRenderbufferParameteriv &&
           gl.GetFramebufferAttachmentParameteriv && gl.GenRenderbuffers &&
           gl.BindRenderbuffer && gl.IsRenderbuffer && gl.GetIntegerv;
}

/* ---- the packed renderbuffers of this process ---- */

struct packed {
    GLuint depth, stencil;
    GLsizei w, h, samples;
};

static struct packed *tab;
static int ntab, captab;
static pthread_mutex_t tab_lock = PTHREAD_MUTEX_INITIALIZER;

static GLint bound_rb(void)
{
    GLint rb = 0;
    gl.GetIntegerv(GL_RENDERBUFFER_BINDING, &rb);
    return rb;
}

static void rb_info(GLuint rb, GLint *fmt, GLint *w, GLint *h, GLint *samples)
{
    gl.BindRenderbuffer(GL_RENDERBUFFER, rb);
    gl.GetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, fmt);
    gl.GetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, w);
    gl.GetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, h);
    *samples = 0;
    if (real_storage_ms)
        gl.GetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES_NV, samples);
}

/* Does entry e describe renderbuffer e->depth of the current context? (binding preserved) */
static int entry_valid(const struct packed *e)
{
    GLint old = bound_rb(), f, w, h, s;
    int ok = 0;

    if (gl.IsRenderbuffer(e->depth) && gl.IsRenderbuffer(e->stencil)) {
        rb_info(e->depth, &f, &w, &h, &s);
        if (f == GL_DEPTH_COMPONENT24_OES && w == e->w && h == e->h && s == e->samples) {
            rb_info(e->stencil, &f, &w, &h, &s);
            ok = f == GL_STENCIL_INDEX8 && w == e->w && h == e->h && s == e->samples;
        }
        gl.BindRenderbuffer(GL_RENDERBUFFER, (GLuint)old);
    }
    return ok;
}

/* The valid entry for depth renderbuffer rb, or NULL. Call with tab_lock held. */
static struct packed *find_depth(GLuint rb)
{
    for (int i = 0; i < ntab; i++)
        if (tab[i].depth == rb && entry_valid(&tab[i]))
            return &tab[i];
    return NULL;
}

static void drop(struct packed *e)
{
    *e = tab[--ntab];
}

/* Storage for the bound renderbuffer; samples < 0 means single-sampled. */
static void storage(GLenum target, GLsizei samples, GLenum fmt, GLsizei w, GLsizei h)
{
    GLint rb = bound_rb();
    struct packed *e = NULL;

    pthread_mutex_lock(&tab_lock);
    for (int i = 0; i < ntab; i++)
        if (tab[i].depth == (GLuint)rb) {
            /* an entry of another share group may hold the same name */
            if (entry_valid(&tab[i])) {
                e = &tab[i];
                break;
            }
        }

    if (fmt != GL_DEPTH24_STENCIL8_OES) {
        if (samples < 0)
            gl.RenderbufferStorage(target, fmt, w, h);
        else
            real_storage_ms(target, samples, fmt, w, h);
        if (e) {
            gl.DeleteRenderbuffers(1, &e->stencil);
            drop(e);
        }
        pthread_mutex_unlock(&tab_lock);
        return;
    }

    if (samples < 0)
        gl.RenderbufferStorage(target, GL_DEPTH_COMPONENT24_OES, w, h);
    else
        real_storage_ms(target, samples, GL_DEPTH_COMPONENT24_OES, w, h);
    if (!rb) {
        pthread_mutex_unlock(&tab_lock);
        return;
    }
    if (!e) {
        if (ntab == captab) {
            int ncap = captab ? captab * 2 : 16;
            struct packed *n = realloc(tab, ncap * sizeof(*n));
            if (!n) {
                pthread_mutex_unlock(&tab_lock);
                return;
            }
            tab = n;
            captab = ncap;
        }
        e = &tab[ntab++];
        e->depth = (GLuint)rb;
        gl.GenRenderbuffers(1, &e->stencil);
    }
    e->w = w;
    e->h = h;
    e->samples = samples < 0 ? 0 : samples;
    gl.BindRenderbuffer(target, e->stencil);
    if (samples < 0)
        gl.RenderbufferStorage(target, GL_STENCIL_INDEX8, w, h);
    else
        real_storage_ms(target, samples, GL_STENCIL_INDEX8, w, h);
    gl.BindRenderbuffer(target, (GLuint)rb);
    pthread_mutex_unlock(&tab_lock);
}

/* ---- the GL entry points ---- */

const GLubyte *glGetString(GLenum name)
{
    static char *ext;
    static pthread_mutex_t ext_lock = PTHREAD_MUTEX_INITIALIZER;
    const GLubyte *s;

    if (!gl_ok())
        return NULL;
    s = gl.GetString(name);
    if (name != GL_EXTENSIONS || !s)
        return s;
    pthread_mutex_lock(&ext_lock);
    if (!ext) {
        size_t n = strlen((const char *)s);
        if (strstr((const char *)s, PACKED_EXT)) {
            ext = (char *)s;
        } else if ((ext = malloc(n + sizeof(PACKED_EXT) + 2))) {
            memcpy(ext, s, n);
            if (n && ext[n - 1] != ' ')
                ext[n++] = ' ';
            memcpy(ext + n, PACKED_EXT, sizeof(PACKED_EXT));
        }
    }
    pthread_mutex_unlock(&ext_lock);
    return ext ? (const GLubyte *)ext : s;
}

void glRenderbufferStorage(GLenum target, GLenum fmt, GLsizei w, GLsizei h)
{
    if (gl_ok())
        storage(target, -1, fmt, w, h);
}

static void storage_ms(GLenum target, GLsizei samples, GLenum fmt, GLsizei w, GLsizei h)
{
    if (gl_ok() && real_storage_ms)
        storage(target, samples, fmt, w, h);
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum rbtarget, GLuint rb)
{
    struct packed *e;
    GLuint stencil = 0;

    if (!gl_ok())
        return;
    if (!rb || (attachment != GL_STENCIL_ATTACHMENT &&
                attachment != GL_DEPTH_STENCIL_ATTACHMENT)) {
        gl.FramebufferRenderbuffer(target, attachment, rbtarget, rb);
        return;
    }
    pthread_mutex_lock(&tab_lock);
    e = find_depth(rb);
    if (e)
        stencil = e->stencil;
    pthread_mutex_unlock(&tab_lock);

    if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        /* ES 3 only; split it for ES 2 */
        gl.FramebufferRenderbuffer(target, GL_DEPTH_ATTACHMENT, rbtarget, rb);
        gl.FramebufferRenderbuffer(target, GL_STENCIL_ATTACHMENT, rbtarget, stencil ? stencil : rb);
        return;
    }
    gl.FramebufferRenderbuffer(target, attachment, rbtarget, stencil ? stencil : rb);
}

void glDeleteRenderbuffers(GLsizei n, const GLuint *rbs)
{
    if (!gl_ok())
        return;
    if (rbs && n > 0) {
        pthread_mutex_lock(&tab_lock);
        for (GLsizei i = 0; i < n; i++) {
            struct packed *e = rbs[i] ? find_depth(rbs[i]) : NULL;
            if (e) {
                gl.DeleteRenderbuffers(1, &e->stencil);
                drop(e);
            }
        }
        pthread_mutex_unlock(&tab_lock);
    }
    gl.DeleteRenderbuffers(n, rbs);
}

void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
    GLint rb;
    struct packed *e;

    if (!gl_ok())
        return;
    gl.GetRenderbufferParameteriv(target, pname, params);
    if (!params || (pname != GL_RENDERBUFFER_INTERNAL_FORMAT &&
                    pname != GL_RENDERBUFFER_STENCIL_SIZE))
        return;
    rb = bound_rb();
    if (!rb)
        return;
    pthread_mutex_lock(&tab_lock);
    e = find_depth((GLuint)rb);
    pthread_mutex_unlock(&tab_lock);
    if (e)
        *params = pname == GL_RENDERBUFFER_INTERNAL_FORMAT ? GL_DEPTH24_STENCIL8_OES : 8;
}

void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname,
                                           GLint *params)
{
    GLint type = 0;

    if (!gl_ok())
        return;
    gl.GetFramebufferAttachmentParameteriv(target, attachment, pname, params);
    if (!params || attachment != GL_STENCIL_ATTACHMENT ||
        pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME || !*params)
        return;
    gl.GetFramebufferAttachmentParameteriv(target, attachment,
                                           GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    if (type != GL_RENDERBUFFER)
        return;
    pthread_mutex_lock(&tab_lock);
    for (int i = 0; i < ntab; i++)
        if (tab[i].stencil == (GLuint)*params && entry_valid(&tab[i])) {
            *params = (GLint)tab[i].depth;
            break;
        }
    pthread_mutex_unlock(&tab_lock);
}

/* Called by libEGL_tegra.so's eglGetProcAddress() for GL extension entry points. */
void *gles2_tegra_wrap_proc(const char *name, void *nv_proc)
{
    if (!name || !nv_proc)
        return NULL;
    if (!strncmp(name, "glRenderbufferStorageMultisample", 32)) {
        real_storage_ms = (storage_ms_fn)nv_proc;
        return (void *)storage_ms;
    }
    return NULL;
}

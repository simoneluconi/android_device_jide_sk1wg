/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * The Android 5.1 GraphicBuffer constructor that NVIDIA's
 * hwcomposer.tegra.so calls, in create_graphic_buffer(), to wrap one of
 * its own scratch buffers:
 *
 *   GraphicBuffer(uint32_t w, uint32_t h, PixelFormat format,
 *                 uint32_t usage, uint32_t stride,
 *                 native_handle_t* handle, bool keepOwnership)
 *
 * The blob allocates the object itself with operator new() and then calls
 * this constructor, so Android 14's GraphicBuffer is built in that memory
 * with the handle-wrapping constructor. The blob was built for 5.1's
 * 128-byte class; extract-files.sh patches its "movs r0, #128" (offset
 * 0xc11c, before the operator new call) to 160, Android 14's size. The
 * rest of what the blob touches has the same layout in both: it calls
 * RefBase::incStrong/decStrong on the object pointer (RefBase at offset
 * 0) and reads the handle at offset 60 of getNativeBuffer(). It passes
 * keepOwnership = false, so the buffer only wraps the handle.
 */

#include <new>
#include <ui/GraphicBuffer.h>

using android::GraphicBuffer;
using android::PixelFormat;

static_assert(sizeof(GraphicBuffer) <= 160,
              "GraphicBuffer no longer fits the 160 bytes the patched "
              "hwcomposer.tegra.so allocates: update extract-files.sh");

extern "C" void _ZN7android13GraphicBufferC1EjjijjP13native_handleb(
        GraphicBuffer *self, uint32_t width, uint32_t height,
        PixelFormat format, uint32_t usage, uint32_t stride,
        native_handle_t *handle, bool keepOwnership)
{
    new (self) GraphicBuffer(handle,
                             keepOwnership ? GraphicBuffer::TAKE_HANDLE
                                           : GraphicBuffer::WRAP_HANDLE,
                             width, height, format, 1 /* layerCount */,
                             static_cast<uint64_t>(usage), stride);
}

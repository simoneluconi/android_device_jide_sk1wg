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
 * The blob allocates the object itself with operator new(128) (the 5.1
 * sizeof) and then calls this constructor, so Android 14's GraphicBuffer
 * is built in that memory with the handle-wrapping constructor. The
 * static_assert guards the size: if Android 14's class grows past 128
 * bytes, the blob's "movs r0, #128" (offset 0xc11c, before the
 * operator new call) has to be patched to a larger size instead.
 */

#include <new>
#include <ui/GraphicBuffer.h>

using android::GraphicBuffer;
using android::PixelFormat;

static_assert(sizeof(GraphicBuffer) <= 128,
              "Android 14 GraphicBuffer no longer fits the 128 bytes "
              "hwcomposer.tegra.so allocates: patch the blob");

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

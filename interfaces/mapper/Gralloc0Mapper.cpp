/*
 * Copyright 2016 The Android Open Source Project
 * * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Gralloc0Mapper"

#include "Gralloc0Mapper.h"

#include <log/log.h>
#include <unistd.h>

namespace {

// sk1wg: NVIDIA's Android 5.1 gralloc.tegra.so keeps the allocating process's
// pid in its native handle at byte offset 0x14 (data[2], after the fd at
// 0x0c). NvGrRegisterBuffer() treats a handle whose pid is the caller's as
// one it allocated itself: it only bumps a refcount and keeps the header page
// mapping (offset 0x80) copied from the original. NvGrUnregisterBuffer() then
// also closes and frees such a handle itself.
//
// Android 14 allocates in the client (passthrough allocator), imports a
// clone through this mapper and frees the original, which unmaps the page
// the clone points at: bootanimation crashed in NvGrLock() on that page + 0x48.
// Clearing the pid on our clone makes every import a full one (its own mmap
// and NvRm handle), as for a buffer from another process, and leaves closing
// and deleting the clone to freeBuffer() as usual.
constexpr int kNvHandleMinWords = (0xb8 - sizeof(native_handle_t)) / sizeof(int);
constexpr int kNvOwnerPidIndex = (0x14 - sizeof(native_handle_t)) / sizeof(int);

void nvClearOwnerPid(buffer_handle_t bufferHandle) {
    native_handle_t* h = const_cast<native_handle_t*>(bufferHandle);
    if (h->version != sizeof(native_handle_t) || h->numFds < 1 ||
        h->numFds > kNvOwnerPidIndex ||
        h->numFds + h->numInts < kNvHandleMinWords) {
        return;
    }
    if (h->data[kNvOwnerPidIndex] == getpid()) {
        h->data[kNvOwnerPidIndex] = 0;
    }
}

}  // namespace

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace implementation {

Gralloc0Mapper::Gralloc0Mapper(const hw_module_t* module)
    : mModule(reinterpret_cast<const gralloc_module_t*>(module)),
      mMinor(module->module_api_version & 0xff) {
    mCapabilities.highUsageBits = false;
    mCapabilities.layeredBuffers = false;
    mCapabilities.unregisterImplyDelete = false;
}

Error Gralloc0Mapper::registerBuffer(buffer_handle_t bufferHandle) {
    // bufferHandle is always our own clone (GrallocMapper::importBuffer)
    nvClearOwnerPid(bufferHandle);
    int result = mModule->registerBuffer(mModule, bufferHandle);
    return result ? Error::BAD_BUFFER : Error::NONE;
}

void Gralloc0Mapper::unregisterBuffer(buffer_handle_t bufferHandle) {
    mModule->unregisterBuffer(mModule, bufferHandle);
}

Error Gralloc0Mapper::lockBuffer(buffer_handle_t bufferHandle,
                                 uint64_t cpuUsage,
                                 const IMapper::Rect& accessRegion, int fenceFd,
                                 void** outData) {
    int result;
    void* data = nullptr;
    if (mMinor >= 3 && mModule->lockAsync) {
        // Dup fenceFd as it is going to be owned by gralloc.  Note that it is
        // gralloc's responsibility to close it, even on locking errors.
        if (fenceFd >= 0) {
            fenceFd = dup(fenceFd);
            if (fenceFd < 0) {
                return Error::NO_RESOURCES;
            }
        }

        result = mModule->lockAsync(mModule, bufferHandle, cpuUsage,
                                    accessRegion.left, accessRegion.top,
                                    accessRegion.width, accessRegion.height,
                                    &data, fenceFd);
    } else {
        waitFenceFd(fenceFd, "Gralloc0Mapper::lock");

        result = mModule->lock(mModule, bufferHandle, cpuUsage,
                               accessRegion.left, accessRegion.top,
                               accessRegion.width, accessRegion.height, &data);
    }

    if (result) {
        return Error::BAD_VALUE;
    } else {
        *outData = data;
        return Error::NONE;
    }
}

Error Gralloc0Mapper::lockBuffer(buffer_handle_t bufferHandle,
                                 uint64_t cpuUsage,
                                 const IMapper::Rect& accessRegion, int fenceFd,
                                 YCbCrLayout* outLayout) {
    int result;
    android_ycbcr ycbcr = {};
    if (mMinor >= 3 && mModule->lockAsync_ycbcr) {
        // Dup fenceFd as it is going to be owned by gralloc.  Note that it is
        // gralloc's responsibility to close it, even on locking errors.
        if (fenceFd >= 0) {
            fenceFd = dup(fenceFd);
            if (fenceFd < 0) {
                return Error::NO_RESOURCES;
            }
        }

        result = mModule->lockAsync_ycbcr(mModule, bufferHandle, cpuUsage,
                                          accessRegion.left, accessRegion.top,
                                          accessRegion.width,
                                          accessRegion.height, &ycbcr, fenceFd);
    } else {
        waitFenceFd(fenceFd, "Gralloc0Mapper::lockYCbCr");

        if (mModule->lock_ycbcr) {
            result = mModule->lock_ycbcr(mModule, bufferHandle, cpuUsage,
                                         accessRegion.left, accessRegion.top,
                                         accessRegion.width,
                                         accessRegion.height, &ycbcr);
        } else {
            result = -EINVAL;
        }
    }

    if (result) {
        return Error::BAD_VALUE;
    } else {
        outLayout->y = ycbcr.y;
        outLayout->cb = ycbcr.cb;
        outLayout->cr = ycbcr.cr;
        outLayout->yStride = ycbcr.ystride;
        outLayout->cStride = ycbcr.cstride;
        outLayout->chromaStep = ycbcr.chroma_step;
        return Error::NONE;
    }
}

Error Gralloc0Mapper::unlockBuffer(buffer_handle_t bufferHandle,
                                   int* outFenceFd) {
    int result;
    int fenceFd = -1;
    if (mMinor >= 3 && mModule->unlockAsync) {
        result = mModule->unlockAsync(mModule, bufferHandle, &fenceFd);
    } else {
        result = mModule->unlock(mModule, bufferHandle);
    }

    if (result) {
        // we always own the fenceFd even when unlock failed
        if (fenceFd >= 0) {
            close(fenceFd);
        }

        return Error::BAD_VALUE;
    } else {
        *outFenceFd = fenceFd;
        return Error::NONE;
    }
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

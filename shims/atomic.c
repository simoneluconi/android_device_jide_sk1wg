/*
 * SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * android_atomic_* for NVIDIA's Android 5.1 gralloc.tegra.so and
 * hwcomposer.tegra.so: libcutils used to export them, Android 14 only
 * has them as inline functions in <cutils/atomic.h>. Same semantics as
 * the old libcutils ones (inc/dec return the previous value, with
 * release ordering; load is acquire, store is release).
 */

#include <stdatomic.h>
#include <stdint.h>

#define A(p) ((volatile atomic_int_least32_t *)(p))

int32_t android_atomic_inc(volatile int32_t *addr)
{
    return atomic_fetch_add_explicit(A(addr), 1, memory_order_release);
}

int32_t android_atomic_dec(volatile int32_t *addr)
{
    return atomic_fetch_sub_explicit(A(addr), 1, memory_order_release);
}

int32_t android_atomic_acquire_load(volatile const int32_t *addr)
{
    return atomic_load_explicit((volatile const atomic_int_least32_t *)addr,
                                memory_order_acquire);
}

void android_atomic_release_store(int32_t value, volatile int32_t *addr)
{
    atomic_store_explicit(A(addr), value, memory_order_release);
}

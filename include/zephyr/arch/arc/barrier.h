/**
 * Copyright (c) 2026 GlobalFoundries Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARC_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_BARRIER_H_

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_H_
#error Please include <zephyr/sys/barrier.h>
#endif

#ifndef __CCAC__
/* ARC GNU GCC lowers the barrier builtins correctly, so it keeps using
 * <zephyr/sys/barrier_builtin.h>. Only MWDT needs this backend.
 */
#error "The ARC barrier backend is a MWDT-only workaround"
#endif

#include <zephyr/toolchain.h>
#include <zephyr/arch/arc/dmb.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MWDT implements __sync_synchronize() as a bare return and emits no dmb for
 * __atomic_thread_fence(), so emit the ARC barrier instructions directly.
 */

static ALWAYS_INLINE void z_barrier_sync_synchronize(void)
{
	__asm__ volatile("dmb " STRINGIFY(ARC_DMB_LOAD_STORE) ::: "memory");
}

static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
	__asm__ volatile("dmb " STRINGIFY(ARC_DMB_LOAD_STORE) ::: "memory");
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
	/* dsync additionally stalls until the prior data accesses completed */
	__asm__ volatile("dsync" ::: "memory");
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
	/* sync is ARC's pipeline-affecting synchronization */
	__asm__ volatile("sync" ::: "memory");
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_BARRIER_H_ */

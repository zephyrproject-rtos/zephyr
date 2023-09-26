/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_BUILTIN_H_
#define ZEPHYR_INCLUDE_SYS_BARRIER_BUILTIN_H_

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_H_
#error Please include <zephyr/sys/barrier.h>
#endif

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
#if defined(__GNUC__)
	/* GCC-ism */
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#else
	atomic_thread_fence(memory_order_seq_cst);
#endif
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
#if defined(__GNUC__)
	/* GCC-ism */
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#else
	atomic_thread_fence(memory_order_seq_cst);
#endif
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
	__asm__ volatile("" : : : "memory");
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_BARRIER_BUILTIN_H_ */

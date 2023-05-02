/**
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BARRIER_ARM64_H_
#define ZEPHYR_INCLUDE_BARRIER_ARM64_H_

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_H_
#error Please include <zephyr/sys/barrier.h>
#endif

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
	__asm__ volatile ("dmb sy" ::: "memory");
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
	__asm__ volatile ("dsb sy" ::: "memory");
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
	__asm__ volatile ("isb" ::: "memory");
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BARRIER_ARM64_H_ */

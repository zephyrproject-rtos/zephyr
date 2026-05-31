/**
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BARRIER_ARM_H_
#define ZEPHYR_INCLUDE_BARRIER_ARM_H_

#ifndef ZEPHYR_INCLUDE_SYS_BARRIER_H_
#error Please include <zephyr/sys/barrier.h>
#endif

#ifdef CONFIG_HAS_CMSIS_CORE
#include <cmsis_core.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void z_barrier_sync_synchronize(void)
{
	__sync_synchronize();
}

static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
#if defined(CONFIG_ARMV6)
	/* ARMv6 has no DMB instruction; use the CP15 c7 operation. */
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory");
#else
	__DMB();
#endif
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
#if defined(CONFIG_ARMV6)
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0) : "memory");
#else
	__DSB();
#endif
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
#if defined(CONFIG_ARMV6)
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 4" : : "r"(0) : "memory");
#else
	__ISB();
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BARRIER_ARM_H_ */

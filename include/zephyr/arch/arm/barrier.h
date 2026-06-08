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

#include <cmsis_core.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void z_barrier_sync_synchronize(void)
{
	__sync_synchronize();
}

#if defined(CONFIG_ARMV6_ARM1176)
/*
 * ARM1176 (ARMv6) does not support the ISB/DSB/DMB mnemonics introduced in
 * ARMv7.  Use the equivalent ARMv6 MCR coprocessor instructions directly so
 * that the third-party CMSIS header (cmsis_gcc.h) does not need to be
 * modified.
 *   ISB: MCR p15, 0, <Rd>, c7, c5, 4  (Flush Prefetch Buffer)
 *   DSB: MCR p15, 0, <Rd>, c7, c10, 4 (Data Synchronization Barrier)
 *   DMB: MCR p15, 0, <Rd>, c7, c10, 5 (Data Memory Barrier)
 * <Rd> is SBZ; the inline assembly passes a zero value and lets the compiler
 * choose the register.
 */
static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory");
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0) : "memory");
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 4" : : "r"(0) : "memory");
}
#else
static ALWAYS_INLINE void z_barrier_dmem_fence_full(void)
{
	__DMB();
}

static ALWAYS_INLINE void z_barrier_dsync_fence_full(void)
{
	__DSB();
}

static ALWAYS_INLINE void z_barrier_isync_fence_full(void)
{
	__ISB();
}
#endif /* CONFIG_ARMV6_ARM1176 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BARRIER_ARM_H_ */

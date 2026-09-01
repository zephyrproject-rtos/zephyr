/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM1176 (ARMv6) CP15 accessors standing in for the CMSIS-Core(A) functions
 * that arm_mmu.c uses. ARM1176 is not a CMSIS target, so cmsis_core.h is not
 * available; these provide the same small set of system-register accessors.
 */

#ifndef ZEPHYR_ARCH_ARM_CORE_MMU_ARMV6_CP15_H_
#define ZEPHYR_ARCH_ARM_CORE_MMU_ARMV6_CP15_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

static ALWAYS_INLINE uint32_t __get_SCTLR(void)
{
	uint32_t val;

	__asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(val));
	return val;
}

static ALWAYS_INLINE void __set_SCTLR(uint32_t val)
{
	__asm__ volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(val) : "memory");
}

static ALWAYS_INLINE void __set_TTBR0(uint32_t val)
{
	__asm__ volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(val) : "memory");
}

static ALWAYS_INLINE void __set_DACR(uint32_t val)
{
	__asm__ volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(val) : "memory");
}

static ALWAYS_INLINE void __set_TLBIALL(uint32_t val)
{
	__asm__ volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(val) : "memory");
}

#endif /* ZEPHYR_ARCH_ARM_CORE_MMU_ARMV6_CP15_H_ */

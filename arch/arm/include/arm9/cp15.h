/*
 * Copyright (C) 2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_ARM9_CP15_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_ARM9_CP15_H_

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t __get_control_register(void)
{
	uint32_t result;

	__asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(result) :: "memory");

	return result;
}

static inline void __set_control_register(uint32_t value)
{
	__asm__ volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(value) : "memory");
}

static inline void __set_TTBR(unsigned int value)
{
	__asm__ volatile("mcr p15, 0, %0, c2, c0, 0" :: "r"(value) : "memory");
}

static inline void __set_DACR(unsigned int value)
{
	__asm__ volatile("mcr p15, 0, %0, c3, c0, 0" :: "r"(value) : "memory");
}

static inline void __set_TLBIALL(uint32_t value)
{
	__asm__ volatile("mcr p15, 0, %0, c8, c7, 0" :: "r"(value) : "memory");
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_ARM9_CP15_H_ */

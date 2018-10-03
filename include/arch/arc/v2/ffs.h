/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 public kernel find-first-set interface
 *
 * ARC-specific kernel ffs interface. Included by arc/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_FFS_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_FFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 */

#if defined(__GNUC__)
static ALWAYS_INLINE unsigned int find_msb_set(u32_t op)
{
	unsigned int bit;

	__asm__ volatile(

		/* see explanation in ffs.S */
		"fls.f %0, %1;\n\t"
		"add.nz %0, %0, 1;\n\t"
		: "=r"(bit)
		: "r"(op));

	return bit;
}
#endif

/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

#if defined(__GNUC__)
static ALWAYS_INLINE unsigned int find_lsb_set(u32_t op)
{
	unsigned int bit;

	__asm__ volatile(

		/* see explanation in ffs.S */
		"ffs.f %0, %1;\n\t"
		"add.nz %0, %0, 1;\n\t"
		"mov.z %0, 0;\n\t"
		: "=&r"(bit)
		: "r"(op));

	return bit;
}
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_FFS_H_ */

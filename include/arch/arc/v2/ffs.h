/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief ARCv2 public nanokernel find-first-set interface
 *
 * ARC-specific nanokernel ffs interface. Included by ARC/arch.h.
 */

#ifndef _ARCH_ARC_V2_FFS_H_
#define _ARCH_ARC_V2_FFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <stdint.h>

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
static ALWAYS_INLINE unsigned int find_msb_set(uint32_t op)
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
static ALWAYS_INLINE unsigned int find_lsb_set(uint32_t op)
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

#endif /* _ARCH_ARC_V2_FFS_H_ */

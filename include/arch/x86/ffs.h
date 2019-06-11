/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief X86 find-first-set interface
 *
 * X86-specific kernel ffs interface
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_FFS_H_
#define ZEPHYR_INCLUDE_ARCH_X86_FFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

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
 *
 * @internal
 * For Intel64 (x86_64) architectures, the 'cmovzl' can be removed and leverage
 * the fact that the 'bsfl' doesn't modify the destination operand when the
 * source operand is zero.  The "bitpos" variable can be preloaded into the
 * destination register, and given the unconditional ++bitpos that is performed
 * after the 'cmovzl', the correct results are yielded.
 */

static ALWAYS_INLINE unsigned int find_lsb_set(u32_t op)
{
	unsigned int bitpos;

	__asm__ volatile (

#ifdef CONFIG_CPU_MINUTEIA

		/* Minute IA doesn't support cmov */

		  "bsfl %1, %0;\n\t"
		  "jnz 1f;\n\t"
		  "movl $-1, %0;\n\t"
		  "1:\n\t"
		: "=r" (bitpos)
		: "rm" (op)
		: "cc"
#else

		"bsfl %1, %0;\n\t"
		"cmovzl %2, %0;\n\t"
		: "=r" (bitpos)
		: "rm" (op), "r" (-1)
		: "cc"

#endif /* CONFIG_CPU_MINUTEIA */
		);

	return (bitpos + 1);
}


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
 *
 * @internal
 * For Intel64 (x86_64) architectures, the 'cmovzl' can be removed and leverage
 * the fact that the 'bsfl' doesn't modify the destination operand when the
 * source operand is zero.  The "bitpos" variable can be preloaded into the
 * destination register, and given the unconditional ++bitpos that is performed
 * after the 'cmovzl', the correct results are yielded.
 */

static ALWAYS_INLINE unsigned int find_msb_set(u32_t op)
{
	unsigned int bitpos;

	__asm__ volatile (

#ifdef CONFIG_CPU_MINUTEIA

		/* again, Minute IA doesn't support cmov */

		  "bsrl %1, %0;\n\t"
		  "jnz 1f;\n\t"
		  "movl $-1, %0;\n\t"
		  "1:\n\t"
		: "=r" (bitpos)
		: "rm" (op)
		: "cc"

#else

		"bsrl %1, %0;\n\t"
		"cmovzl %2, %0;\n\t"
		: "=r" (bitpos)
		: "rm" (op), "r" (-1)

#endif /* CONFIG_CPU_MINUTEIA */
		);

	return (bitpos + 1);
}



#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_X86_FFS_H_ */

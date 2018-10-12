/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV32_PULPINO_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV32_PULPINO_ASM_INLINE_GCC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 * NOTE: only pulpino-specific NOT RISCV32
 */

#ifndef _ASMLANGUAGE

#include <toolchain.h>

/*
 * Account for pulpino-specific bit manipulation opcodes only when
 * CONFIG_RISCV_GENERIC_TOOLCHAIN is not set
 */

#ifndef CONFIG_RISCV_GENERIC_TOOLCHAIN
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
static ALWAYS_INLINE unsigned int find_lsb_set(u32_t op)
{
	unsigned int ret;

	if (!op)
		return 0;

	__asm__ volatile ("p.ff1 %[d], %[a]"
			  : [d] "=r" (ret)
			  : [a] "r" (op));

	return ret + 1;
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
 */
static ALWAYS_INLINE unsigned int find_msb_set(u32_t op)
{
	unsigned int ret;

	if (!op)
		return 0;

	__asm__ volatile ("p.fl1 %[d], %[a]"
			  : [d] "=r" (ret)
			  : [a] "r" (op));

	return ret + 1;
}

#else /* CONFIG_RISCV_GENERIC_TOOLCHAIN */

/*
 * When compiled with a riscv32 generic toolchain, use
 * __builtin_ffs and __builtin_clz to handle respectively
 * find_lsb_set and find_msb_set.
 */
static ALWAYS_INLINE unsigned int find_lsb_set(u32_t op)
{
	return __builtin_ffs(op);
}

static ALWAYS_INLINE unsigned int find_msb_set(u32_t op)
{
	if (!op)
		return 0;

	return 32 - __builtin_clz(op);
}

#endif /* CONFIG_RISCV_GENERIC_TOOLCHAIN */

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */

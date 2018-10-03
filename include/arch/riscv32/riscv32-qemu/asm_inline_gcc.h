/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV32_RISCV32_QEMU_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV32_RISCV32_QEMU_ASM_INLINE_GCC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 * riscv32-qemu does not have bit manipulation asm opcodes.
 * Handle find_lsb_set and find_msb_set in C.
 */

#ifndef _ASMLANGUAGE

#include <toolchain.h>

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
	return __builtin_ffs(op);
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
	if (!op)
		return 0;

	return 32 - __builtin_clz(op);
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV32_RISCV32_QEMU_ASM_INLINE_GCC_H_ */

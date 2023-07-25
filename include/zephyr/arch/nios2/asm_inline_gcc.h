/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_NIOS2_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_NIOS2_ASM_INLINE_GCC_H_

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/toolchain.h>

/* Using the *io variants of these instructions to prevent issues on
 * devices that have an instruction/data cache
 */

static ALWAYS_INLINE void sys_write32(uint32_t data, mm_reg_t addr)
{
	__builtin_stwio((void *)addr, data);
}

static ALWAYS_INLINE uint32_t sys_read32(mm_reg_t addr)
{
	return __builtin_ldwio((void *)addr);
}

static ALWAYS_INLINE void sys_write8(uint8_t data, mm_reg_t addr)
{
	sys_write32(data, addr);
}

static ALWAYS_INLINE uint8_t sys_read8(mm_reg_t addr)
{
	return __builtin_ldbuio((void *)addr);
}

static ALWAYS_INLINE void sys_write16(uint16_t data, mm_reg_t addr)
{
	sys_write32(data, addr);
}

static ALWAYS_INLINE uint16_t sys_read16(mm_reg_t addr)
{
	return __builtin_ldhuio((void *)addr);
}

#endif /* _ASMLANGUAGE */

#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */

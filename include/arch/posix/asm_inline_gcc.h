/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 * Copyright (c) 2017, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * POSIX ARCH specific public inline "assembler" functions and macros
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_GCC_H_

/*
 * The file must not be included directly
 * Include kernel.h instead
 */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <toolchain/common.h>
#include <zephyr/types.h>
#include <sys_io.h>
#include "posix_soc_if.h"

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
	if (!op) {
		return 0;
	}

	return 32 - __builtin_clz(op);
}


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


static ALWAYS_INLINE u8_t sys_read8(mem_addr_t addr)
{
	return *(volatile u8_t *)addr;
}

static ALWAYS_INLINE void sys_write8(u8_t data, mem_addr_t addr)
{
	*(volatile u8_t *)addr = data;
}

static ALWAYS_INLINE u16_t sys_read16(mem_addr_t addr)
{
	return *(volatile u16_t *)addr;
}

static ALWAYS_INLINE void sys_write16(u16_t data, mem_addr_t addr)
{
	*(volatile u16_t *)addr = data;
}

static ALWAYS_INLINE u32_t sys_read32(mem_addr_t addr)
{
	return *(volatile u32_t *)addr;
}

static ALWAYS_INLINE void sys_write32(u32_t data, mem_addr_t addr)
{
	*(volatile u32_t *)addr = data;
}

/* Memory bit manipulation functions */

static ALWAYS_INLINE void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	*(volatile u32_t *)addr = temp | (1 << bit);
}

static ALWAYS_INLINE void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	*(volatile u32_t *)addr = temp & ~(1 << bit);
}

static ALWAYS_INLINE int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t temp = *(volatile u32_t *)addr;

	return temp & (1 << bit);
}

static ALWAYS_INLINE
	void sys_bitfield_set_bit(mem_addr_t addr, unsigned int bit)
{
	/* Doing memory offsets in terms of 32-bit values to prevent
	 * alignment issues
	 */
	sys_set_bit(addr + ((bit >> 5) << 2), bit & 0x1F);
}

static ALWAYS_INLINE
	void sys_bitfield_clear_bit(mem_addr_t addr, unsigned int bit)
{
	sys_clear_bit(addr + ((bit >> 5) << 2), bit & 0x1F);
}

static ALWAYS_INLINE
	int sys_bitfield_test_bit(mem_addr_t addr, unsigned int bit)
{
	return sys_test_bit(addr + ((bit >> 5) << 2), bit & 0x1F);
}

static ALWAYS_INLINE
	int sys_test_and_set_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = sys_test_bit(addr, bit);
	sys_set_bit(addr, bit);

	return ret;
}

static ALWAYS_INLINE
	int sys_test_and_clear_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = sys_test_bit(addr, bit);
	sys_clear_bit(addr, bit);

	return ret;
}

static ALWAYS_INLINE
	int sys_bitfield_test_and_set_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = sys_bitfield_test_bit(addr, bit);
	sys_bitfield_set_bit(addr, bit);

	return ret;
}

static ALWAYS_INLINE
	int sys_bitfield_test_and_clear_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	ret = sys_bitfield_test_bit(addr, bit);
	sys_bitfield_clear_bit(addr, bit);

	return ret;
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_ASM_INLINE_GCC_H_ */

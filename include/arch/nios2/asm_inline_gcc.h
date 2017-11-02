/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ASM_INLINE_GCC_H
#define _ASM_INLINE_GCC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE

#include <sys_io.h>
#include <toolchain.h>

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

/* Using the *io variants of these instructions to prevent issues on
 * devices that have an instruction/data cache
 */

static ALWAYS_INLINE
	void sys_write32(u32_t data, mm_reg_t addr)
{
	__builtin_stwio((void *)addr, data);
}

static ALWAYS_INLINE
	u32_t sys_read32(mm_reg_t addr)
{
	return __builtin_ldwio((void *)addr);
}

static ALWAYS_INLINE
	void sys_write8(u8_t data, mm_reg_t addr)
{
	sys_write32(data, addr);
}

static ALWAYS_INLINE
	u8_t sys_read8(mm_reg_t addr)
{
	return __builtin_ldbuio((void *)addr);
}

static ALWAYS_INLINE
	void sys_write16(u16_t data, mm_reg_t addr)
{
	sys_write32(data, addr);
}

static ALWAYS_INLINE
	u16_t sys_read16(mm_reg_t addr)
{
	return __builtin_ldhuio((void *)addr);
}

/* NIOS II does not have any special instructions for manipulating bits,
 * so just read, modify, write in C
 */

static ALWAYS_INLINE
	void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	sys_write32(sys_read32(addr) | (1 << bit), addr);
}

static ALWAYS_INLINE
	void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	sys_write32(sys_read32(addr) & ~(1 << bit), addr);
}

static ALWAYS_INLINE
	int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	return sys_read32(addr) & (1 << bit);
}

/* These are not required to be atomic, just do it in C */

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

#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */

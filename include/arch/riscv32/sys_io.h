/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief RISCV32 memory mapped register I/O operations
 */

#ifndef _RISCV32_SYS_IO_H_
#define _RISCV32_SYS_IO_H_

#if !defined(_ASMLANGUAGE)

#include <sys_io.h>

/* Memory mapped registers I/O functions */

static inline uint32_t sys_read32(mem_addr_t addr)
{
	return *(volatile uint32_t *)addr;
}


static inline void sys_write32(uint32_t data, mem_addr_t addr)
{
	*(volatile uint32_t *)addr = data;
}

static inline uint8_t sys_read8(mem_addr_t addr)
{
	return *(volatile uint8_t *)addr;
}

static inline void sys_write8(uint8_t data, mem_addr_t addr)
{
	*(volatile uint8_t *)addr = data;
}

/* Memory bit manipulation functions */

static inline void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp | (1 << bit);
}

static inline void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp & ~(1 << bit);
}

static ALWAYS_INLINE
	int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	return temp & (1 << bit);
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

#endif /* !_ASMLANGUAGE */

#endif /* _RISCV32_SYS_IO_H_ */

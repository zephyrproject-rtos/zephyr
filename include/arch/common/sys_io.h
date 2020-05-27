/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 * Copyright (c) 2017, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* "Arch" bit manipulation functions in non-arch-specific C code (uses some
 * gcc builtins)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_SYS_IO_H_

#ifndef _ASMLANGUAGE

#include <toolchain.h>
#include <zephyr/types.h>
#include <sys/sys_io.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE uint8_t sys_read8(mem_addr_t addr)
{
	return *(volatile uint8_t *)addr;
}

static ALWAYS_INLINE void sys_write8(uint8_t data, mem_addr_t addr)
{
	*(volatile uint8_t *)addr = data;
}

static ALWAYS_INLINE uint16_t sys_read16(mem_addr_t addr)
{
	return *(volatile uint16_t *)addr;
}

static ALWAYS_INLINE void sys_write16(uint16_t data, mem_addr_t addr)
{
	*(volatile uint16_t *)addr = data;
}

static ALWAYS_INLINE uint32_t sys_read32(mem_addr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static ALWAYS_INLINE void sys_write32(uint32_t data, mem_addr_t addr)
{
	*(volatile uint32_t *)addr = data;
}

/* Memory bit manipulation functions */

static ALWAYS_INLINE void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp | (1 << bit);
}

static ALWAYS_INLINE void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

	*(volatile uint32_t *)addr = temp & ~(1 << bit);
}

static ALWAYS_INLINE int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	uint32_t temp = *(volatile uint32_t *)addr;

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

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_SYS_IO_H_ */

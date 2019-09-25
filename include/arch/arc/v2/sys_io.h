/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_SYS_IO_H_

#ifndef _ASMLANGUAGE

#include <sys/sys_io.h>
#include <arch/arc/v2/aux_regs.h>

#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Implementation of sys_io.h's documented functions */

static ALWAYS_INLINE
	void sys_out8(u8_t data, io_port_t port)
{
	z_arc_v2_aux_reg_write(port, data);
}

static ALWAYS_INLINE
	u8_t sys_in8(io_port_t port)
{
	return (u8_t)(z_arc_v2_aux_reg_read(port) & 0x000000ff);
}

static ALWAYS_INLINE
	void sys_out16(u16_t data, io_port_t port)
{
	z_arc_v2_aux_reg_write(port, data);
}

static ALWAYS_INLINE
	u16_t sys_in16(io_port_t port)
{
	return (u16_t)(z_arc_v2_aux_reg_read(port) & 0x0000ffff);
}

static ALWAYS_INLINE
	void sys_out32(u32_t data, io_port_t port)
{
	z_arc_v2_aux_reg_write(port, data);
}

static ALWAYS_INLINE
	u32_t sys_in32(io_port_t port)
{
	return z_arc_v2_aux_reg_read(port);
}

static ALWAYS_INLINE
	void sys_io_set_bit(io_port_t port, unsigned int bit)
{
	u32_t reg = 0;

	__asm__ volatile("lr	%1, [%0]\n"
			 "bset	%1, %1, %2\n"
			 "sr	%1, [%0];\n\t"
			 :
			 : "ir" (port),
			   "r" (reg), "Mr" (bit)
			 : "memory", "cc");
}

static ALWAYS_INLINE
	void sys_io_clear_bit(io_port_t port, unsigned int bit)
{
	u32_t reg = 0;

	__asm__ volatile("lr	%1, [%0]\n"
			 "bclr	%1, %1, %2\n"
			 "sr	%1, [%0];\n\t"
			 :
			 : "ir" (port),
			   "r" (reg), "Mr" (bit)
			 : "memory", "cc");
}

static ALWAYS_INLINE
	int sys_io_test_bit(io_port_t port, unsigned int bit)
{
	u32_t status = _ARC_V2_STATUS32;
	u32_t reg = 0;
	u32_t ret;

	__asm__ volatile("lr	%2, [%1]\n"
			 "btst	%2, %3\n"
			 "lr	%0, [%4];\n\t"
			 : "=r" (ret)
			 : "ir" (port),
			   "r" (reg), "Mr" (bit), "i" (status)
			 : "memory", "cc");

	return !(ret & _ARC_V2_STATUS32_Z);
}

static ALWAYS_INLINE
	int sys_io_test_and_set_bit(io_port_t port, unsigned int bit)
{
	int ret;

	ret = sys_io_test_bit(port, bit);
	sys_io_set_bit(port, bit);

	return ret;
}

static ALWAYS_INLINE
	int sys_io_test_and_clear_bit(io_port_t port, unsigned int bit)
{
	int ret;

	ret = sys_io_test_bit(port, bit);
	sys_io_clear_bit(port, bit);

	return ret;
}

static ALWAYS_INLINE
	void sys_write8(u8_t data, mm_reg_t addr)
{
	__asm__ volatile("stb%U1	%0, %1;\n\t"
			 :
			 : "r" (data), "m" (*(volatile u8_t *) addr)
			 : "memory");
}

static ALWAYS_INLINE
	u8_t sys_read8(mm_reg_t addr)
{
	u8_t ret;

	__asm__ volatile("ldb%U1	%0, %1;\n\t"
			 : "=r" (ret)
			 : "m" (*(volatile u8_t *) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE
	void sys_write16(u16_t data, mm_reg_t addr)
{
	__asm__ volatile("sth%U1	%0, %1;\n\t"
			 :
			 : "r" (data), "m" (*(volatile u16_t *) addr)
			 : "memory");
}

static ALWAYS_INLINE
	u16_t sys_read16(mm_reg_t addr)
{
	u16_t ret;

	__asm__ volatile("ldh%U1	%0, %1;\n\t"
			 : "=r" (ret)
			 : "m" (*(volatile u16_t *) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE
	void sys_write32(u32_t data, mm_reg_t addr)
{
	__asm__ volatile("st%U1	%0, %1;\n\t"
			 :
			 : "r" (data), "m" (*(volatile u32_t *) addr)
			 : "memory");
}

static ALWAYS_INLINE
	u32_t sys_read32(mm_reg_t addr)
{
	u32_t ret;

	__asm__ volatile("ld%U1	%0, %1;\n\t"
			 : "=r" (ret)
			 : "m" (*(volatile u32_t *) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE
	void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t reg = 0;

	__asm__ volatile("ld	%1, %0\n"
			 "bset	%1, %1, %2\n"
			 "st	%1, %0;\n\t"
			 : "+m" (*(volatile u32_t *) addr)
			 : "r" (reg), "Mr" (bit)
			 : "memory", "cc");
}

static ALWAYS_INLINE
	void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t reg = 0;

	__asm__ volatile("ld	%1, %0\n"
			 "bclr	%1, %1, %2\n"
			 "st	%1, %0;\n\t"
			 : "+m" (*(volatile u32_t *) addr)
			 : "r" (reg), "Mr" (bit)
			 : "memory", "cc");
}

static ALWAYS_INLINE
	int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	u32_t status = _ARC_V2_STATUS32;
	u32_t reg = 0;
	u32_t ret;

	__asm__ volatile("ld	%2, %1\n"
			 "btst	%2, %3\n"
			 "lr	%0, [%4];\n\t"
			 : "=r" (ret)
			 : "m" (*(volatile u32_t *) addr),
			   "r" (reg), "Mr" (bit), "i" (status)
			 : "memory", "cc");

	return !(ret & _ARC_V2_STATUS32_Z);
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

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_SYS_IO_H_ */

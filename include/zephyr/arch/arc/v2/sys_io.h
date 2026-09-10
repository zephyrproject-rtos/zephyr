/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_SYS_IO_H_

#ifndef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/arch/arc/v2/aux_regs.h>

#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Implementation of sys_io.h's documented functions */

static ALWAYS_INLINE
	void sys_out8(uint8_t data, io_port_t port)
{
	z_arc_v2_aux_reg_write(port, data);
}

static ALWAYS_INLINE
	uint8_t sys_in8(io_port_t port)
{
	return (uint8_t)(z_arc_v2_aux_reg_read(port) & 0x000000ff);
}

static ALWAYS_INLINE
	void sys_out16(uint16_t data, io_port_t port)
{
	z_arc_v2_aux_reg_write(port, data);
}

static ALWAYS_INLINE
	uint16_t sys_in16(io_port_t port)
{
	return (uint16_t)(z_arc_v2_aux_reg_read(port) & 0x0000ffff);
}

static ALWAYS_INLINE
	void sys_out32(uint32_t data, io_port_t port)
{
	z_arc_v2_aux_reg_write(port, data);
}

static ALWAYS_INLINE
	uint32_t sys_in32(io_port_t port)
{
	return z_arc_v2_aux_reg_read(port);
}

static ALWAYS_INLINE
	void sys_io_set_bit(io_port_t port, unsigned int bit)
{
	uint32_t reg = 0;

	__asm__ volatile("lr	%1, [%0]\n"
			 "bset	%1, %1, %2\n"
			 "sr	%1, [%0];\n\t"
			 :
			 : "ir" (port),
			   "r" (reg), "ir" (bit)
			 : "memory", "cc");
}

static ALWAYS_INLINE
	void sys_io_clear_bit(io_port_t port, unsigned int bit)
{
	uint32_t reg = 0;

	__asm__ volatile("lr	%1, [%0]\n"
			 "bclr	%1, %1, %2\n"
			 "sr	%1, [%0];\n\t"
			 :
			 : "ir" (port),
			   "r" (reg), "ir" (bit)
			 : "memory", "cc");
}

static ALWAYS_INLINE
	int sys_io_test_bit(io_port_t port, unsigned int bit)
{
	uint32_t status = _ARC_V2_STATUS32;
	uint32_t reg = 0;
	uint32_t ret;

	__asm__ volatile("lr	%2, [%1]\n"
			 "btst	%2, %3\n"
			 "lr	%0, [%4];\n\t"
			 : "=r" (ret)
			 : "ir" (port),
			   "r" (reg), "ir" (bit), "i" (status)
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

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_SYS_IO_H_ */

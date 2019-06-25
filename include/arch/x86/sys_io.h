/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Implementation of sys_io.h's documented functions */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_X86_SYS_IO_H_


#if !defined(_ASMLANGUAGE)

#include <sys/sys_io.h>
#include <zephyr/types.h>
#include <stddef.h>

static ALWAYS_INLINE
void sys_out8(u8_t data, io_port_t port)
{
	__asm__ volatile("outb	%b0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static ALWAYS_INLINE
	u8_t sys_in8(io_port_t port)
{
	u8_t ret;

	__asm__ volatile("inb	%w1, %b0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static ALWAYS_INLINE
	void sys_out16(u16_t data, io_port_t port)
{
	__asm__ volatile("outw	%w0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static ALWAYS_INLINE
	u16_t sys_in16(io_port_t port)
{
	u16_t ret;

	__asm__ volatile("inw	%w1, %w0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static ALWAYS_INLINE
	void sys_out32(u32_t data, io_port_t port)
{
	__asm__ volatile("outl	%0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static ALWAYS_INLINE
	u32_t sys_in32(io_port_t port)
{
	u32_t ret;

	__asm__ volatile("inl	%w1, %0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static ALWAYS_INLINE
	void sys_io_set_bit(io_port_t port, unsigned int bit)
{
	u32_t reg = 0;

	__asm__ volatile("inl	%w1, %0;\n\t"
			 "btsl	%2, %0;\n\t"
			 "outl	%0, %w1;\n\t"
			 :
			 : "a" (reg), "Nd" (port), "Ir" (bit));
}

static ALWAYS_INLINE
	void sys_io_clear_bit(io_port_t port, unsigned int bit)
{
	u32_t reg = 0;

	__asm__ volatile("inl	%w1, %0;\n\t"
			 "btrl	%2, %0;\n\t"
			 "outl	%0, %w1;\n\t"
			 :
			 : "a" (reg), "Nd" (port), "Ir" (bit));
}

static ALWAYS_INLINE
	int sys_io_test_bit(io_port_t port, unsigned int bit)
{
	u32_t ret;

	__asm__ volatile("inl	%w1, %0\n\t"
			 "btl	%2, %0\n\t"
			 : "=a" (ret)
			 : "Nd" (port), "Ir" (bit));

	return (ret & 1U);
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
	__asm__ volatile("movb	%0, %1;\n\t"
			 :
			 : "q"(data), "m" (*(volatile u8_t *) addr)
			 : "memory");
}

static ALWAYS_INLINE
	u8_t sys_read8(mm_reg_t addr)
{
	u8_t ret;

	__asm__ volatile("movb	%1, %0;\n\t"
			 : "=q"(ret)
			 : "m" (*(volatile u8_t *) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE
	void sys_write16(u16_t data, mm_reg_t addr)
{
	__asm__ volatile("movw	%0, %1;\n\t"
			 :
			 : "r"(data), "m" (*(volatile u16_t *) addr)
			 : "memory");
}

static ALWAYS_INLINE
	u16_t sys_read16(mm_reg_t addr)
{
	u16_t ret;

	__asm__ volatile("movw	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile u16_t *) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE
	void sys_write32(u32_t data, mm_reg_t addr)
{
	__asm__ volatile("movl	%0, %1;\n\t"
			 :
			 : "r"(data), "m" (*(volatile u32_t *) addr)
			 : "memory");
}

static ALWAYS_INLINE
	u32_t sys_read32(mm_reg_t addr)
{
	u32_t ret;

	__asm__ volatile("movl	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile u32_t *) addr)
			 : "memory");

	return ret;
}


static ALWAYS_INLINE
	void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	__asm__ volatile("btsl	%1, %0;\n\t"
			 : "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit)
			 : "memory");
}

static ALWAYS_INLINE
	void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	__asm__ volatile("btrl	%1, %0;\n\t"
			 : "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));
}

static ALWAYS_INLINE
	int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	__asm__ volatile("btl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

static ALWAYS_INLINE
	int sys_test_and_set_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	__asm__ volatile("btsl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

static ALWAYS_INLINE
	int sys_test_and_clear_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	__asm__ volatile("btrl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

#define sys_bitfield_set_bit sys_set_bit
#define sys_bitfield_clear_bit sys_clear_bit
#define sys_bitfield_test_bit sys_test_bit
#define sys_bitfield_test_and_set_bit sys_test_and_set_bit
#define sys_bitfield_test_and_clear_bit sys_test_and_clear_bit


#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_SYS_IO_H_ */

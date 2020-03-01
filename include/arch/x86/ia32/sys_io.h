/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Implementation of sys_io.h's documented functions */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_IA32_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_X86_IA32_SYS_IO_H_

#if !defined(_ASMLANGUAGE)

#include <sys/sys_io.h>
#include <zephyr/types.h>
#include <stddef.h>

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

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_IA32_SYS_IO_H_ */

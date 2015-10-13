/* asm_inline_gcc.h - ARC inline assembler and macros for public functions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ASM_INLINE_GCC_H__
#define __ASM_INLINE_GCC_H__

#ifndef _ASMLANGUAGE

#include <sys_io.h>
#include <arch/arc/v2/aux_regs.h>

#include <stdint.h>
#include <stddef.h>

/* Implementation of sys_io.h's documented functions */

static inline __attribute__((always_inline))
	void sys_out8(uint8_t data, io_port_t port)
{
	_arc_v2_aux_reg_write(port, data);
}

static inline __attribute__((always_inline))
	uint8_t sys_in8(io_port_t port)
{
	return (uint8_t)(_arc_v2_aux_reg_read(port) & 0x000000ff);
}

static inline __attribute__((always_inline))
	void sys_out16(uint16_t data, io_port_t port)
{
	_arc_v2_aux_reg_write(port, data);
}

static inline __attribute__((always_inline))
	uint16_t sys_in16(io_port_t port)
{
	return (uint16_t)(_arc_v2_aux_reg_read(port) & 0x0000ffff);
}

static inline __attribute__((always_inline))
	void sys_out32(uint32_t data, io_port_t port)
{
	_arc_v2_aux_reg_write(port, data);
}

static inline __attribute__((always_inline))
	uint32_t sys_in32(io_port_t port)
{
	return _arc_v2_aux_reg_read(port);
}

static inline __attribute__((always_inline))
	void sys_write8(uint8_t data, mm_reg_t addr)
{
	__asm__ volatile("stb%U1	%0, %1;\n\t"
			 :
			 : "r" (data), "m" (*(volatile uint8_t *) addr)
			 : "memory");
}

static inline __attribute__((always_inline))
	uint8_t sys_read8(mm_reg_t addr)
{
	uint8_t ret;

	__asm__ volatile("ldb%U1	%0, %1;\n\t"
			 : "=r" (ret)
			 : "m" (*(volatile uint8_t *) addr)
			 : "memory");

	return ret;
}

static inline __attribute__((always_inline))
	void sys_write16(uint16_t data, mm_reg_t addr)
{
	__asm__ volatile("sth%U1	%0, %1;\n\t"
			 :
			 : "r" (data), "m" (*(volatile uint16_t *) addr)
			 : "memory");
}

static inline __attribute__((always_inline))
	uint16_t sys_read16(mm_reg_t addr)
{
	uint16_t ret;

	__asm__ volatile("ldh%U1	%0, %1;\n\t"
			 : "=r" (ret)
			 : "m" (*(volatile uint16_t *) addr)
			 : "memory");

	return ret;
}

static inline __attribute__((always_inline))
	void sys_write32(uint32_t data, mm_reg_t addr)
{
	__asm__ volatile("st%U1	%0, %1;\n\t"
			 :
			 : "r" (data), "m" (*(volatile uint32_t *) addr)
			 : "memory");
}

static inline __attribute__((always_inline))
	uint32_t sys_read32(mm_reg_t addr)
{
	uint32_t ret;

	__asm__ volatile("ld%U1	%0, %1;\n\t"
			 : "=r" (ret)
			 : "m" (*(volatile uint32_t *) addr)
			 : "memory");

	return ret;
}

static inline __attribute__((always_inline))
	void sys_set_bit(mem_addr_t addr, int bit)
{
	uint32_t reg = 0;

	__asm__ volatile("ld	%1, %0\n"
			 "bset	%1, %1, %2\n"
			 "st	%1, %0;\n\t"
			 : "+m" (*(volatile uint32_t *) addr)
			 : "r" (reg), "Mr" (bit)
			 : "memory", "cc");
}

static inline __attribute__((always_inline))
	void sys_clear_bit(mem_addr_t addr, int bit)
{
	uint32_t reg = 0;

	__asm__ volatile("ld	%1, %0\n"
			 "bclr	%1, %1, %2\n"
			 "st	%1, %0;\n\t"
			 : "+m" (*(volatile uint32_t *) addr)
			 : "r" (reg), "Mr" (bit)
			 : "memory", "cc");
}

static inline __attribute__((always_inline))
	int sys_test_bit(mem_addr_t addr, int bit)
{
	uint32_t status = _ARC_V2_STATUS32;
	uint32_t reg = 0;
	uint32_t ret;

	__asm__ volatile("ld	%2, %1\n"
			 "btst	%2, %3\n"
			 "lr	%0, [%4];\n\t"
			 : "=r" (ret)
			 : "m" (*(volatile uint32_t *) addr),
			   "r" (reg), "Mr" (bit), "i" (status)
			 : "memory", "cc");

	return !(ret & _ARC_V2_STATUS32_Z);
}

static inline __attribute__((always_inline))
	int sys_test_and_set_bit(mem_addr_t addr, int bit)
{
	int ret;

	ret = sys_test_bit(addr, bit);
	sys_set_bit(addr, bit);

	return ret;
}

static inline __attribute__((always_inline))
	int sys_test_and_clear_bit(mem_addr_t addr, int bit)
{
	int ret;

	ret = sys_test_bit(addr, bit);
	sys_clear_bit(addr, bit);

	return ret;
}

#endif /* _ASMLANGUAGE */
#endif /* __ASM_INLINE_GCC_H__ */

/* asm_inline_gcc.h - ARC inline assembler and macros for public functions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

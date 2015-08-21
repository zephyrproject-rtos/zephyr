/* Intel x86 GCC specific public inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
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
 * 3) Neither the name of Wind River Systems nor the names of its contributors
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

/* Either public functions or macros or invoked by public functions */

#ifndef _ASM_INLINE_GCC_PUBLIC_GCC_H
#define _ASM_INLINE_GCC_PUBLIC_GCC_H

/*
 * The file must not be included directly
 * Include nanokernel/cpu.h instead
 */

#include <sys_io.h>

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stddef.h>

/**
 *
 * @internal
 *
 * @brief Disable all interrupts on the CPU
 *
 * GCC assembly internals of irq_lock(). See irq_lock() for a complete
 * description.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 */

static inline __attribute__((always_inline)) unsigned int _do_irq_lock(void)
{
	unsigned int key;

	__asm__ volatile (
		"pushfl;\n\t"
		"cli;\n\t"
		"popl %0;\n\t"
		: "=g" (key)
		:
		: "memory"
		);

	return key;
}


/**
 *
 * @internal
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * GCC assembly internals of irq_lock_unlock(). See irq_lock_unlock() for a
 * complete description.
 *
 * @return N/A
 */

static inline __attribute__((always_inline)) void _do_irq_unlock(void)
{
	__asm__ volatile (
		"sti;\n\t"
		: :
		);
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
 *
 * @internal
 * For Intel64 (x86_64) architectures, the 'cmovzl' can be removed and leverage
 * the fact that the 'bsfl' doesn't modify the destination operand when the
 * source operand is zero.  The "bitpos" variable can be preloaded into the
 * destination register, and given the unconditional ++bitpos that is performed
 * after the 'cmovzl', the correct results are yielded.
 */

static ALWAYS_INLINE unsigned int find_lsb_set(uint32_t op)
{
	int bitpos;

	__asm__ volatile (

#if defined(CONFIG_CMOV)

		"bsfl %1, %0;\n\t"
		"cmovzl %2, %0;\n\t"
		: "=r" (bitpos)
		: "rm" (op), "r" (-1)
		: "cc"

#else

		  "bsfl %1, %0;\n\t"
		  "jnz 1f;\n\t"
		  "movl $-1, %0;\n\t"
		  "1:\n\t"
		: "=r" (bitpos)
		: "rm" (op)
		: "cc"

#endif /* CONFIG_CMOV */
		);

	return (bitpos + 1);
}


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
 *
 * @internal
 * For Intel64 (x86_64) architectures, the 'cmovzl' can be removed and leverage
 * the fact that the 'bsfl' doesn't modify the destination operand when the
 * source operand is zero.  The "bitpos" variable can be preloaded into the
 * destination register, and given the unconditional ++bitpos that is performed
 * after the 'cmovzl', the correct results are yielded.
 */

static ALWAYS_INLINE unsigned int find_msb_set(uint32_t op)
{
	int bitpos;

	__asm__ volatile (

#if defined(CONFIG_CMOV)

		"bsrl %1, %0;\n\t"
		"cmovzl %2, %0;\n\t"
		: "=r" (bitpos)
		: "rm" (op), "r" (-1)

#else

		  "bsrl %1, %0;\n\t"
		  "jnz 1f;\n\t"
		  "movl $-1, %0;\n\t"
		  "1:\n\t"
		: "=r" (bitpos)
		: "rm" (op)
		: "cc"

#endif /* CONFIG_CMOV */
		);

	return (bitpos + 1);
}


/**
 *
 *  _NanoTscRead - read timestamp register ensuring serialization
 */

static inline uint64_t _NanoTscRead(void)
{
	union {
		struct  {
			uint32_t lo;
			uint32_t hi;
		};
		uint64_t  value;
	}  rv;

	/* rdtsc & cpuid clobbers eax, ebx, ecx and edx registers */
	__asm__ volatile (/* serialize */
		"xorl %%eax,%%eax;\n\t"
		"cpuid;\n\t"
		:
		:
		: "%eax", "%ebx", "%ecx", "%edx"
		);
	/*
	 * We cannot use "=A", since this would use %rax on x86_64 and
	 * return only the lower 32bits of the TSC
	 */
	__asm__ volatile ("rdtsc" : "=a" (rv.lo), "=d" (rv.hi));


	return rv.value;
}


/**
 *
 * @brief Get a 32 bit CPU timestamp counter
 *
 * @return a 32-bit number
 */

static inline __attribute__((always_inline))
	uint32_t _do_read_cpu_timestamp32(void)
{
	uint32_t rv;

	__asm__ volatile("rdtsc" : "=a"(rv) :  : "%edx");

	return rv;
}


/* Implementation of sys_io.h's documented functions */

static inline __attribute__((always_inline))
	void sys_out8(uint8_t data, io_port_t port)
{
	__asm__ volatile("outb	%%al, %%dx;\n\t" : : "a"(data), "d"(port));
}


static inline __attribute__((always_inline))
	uint8_t sys_in8(io_port_t port)
{
	uint8_t ret;

	__asm__ volatile("inb	%%dx, %%al;\n\t" : "=a"(ret) : "d"(port));
	return ret;
}


static inline __attribute__((always_inline))
	void sys_out16(uint16_t data, io_port_t port)
{
	__asm__ volatile("outw	%%ax, %%dx;\n\t" :  : "a"(data), "d"(port));
}


static inline inline __attribute__((always_inline))
	uint16_t sys_in16(io_port_t port)
{
	uint16_t ret;

	__asm__ volatile("inw	%%dx, %%ax;\n\t" : "=a"(ret) : "d"(port));
	return ret;
}


static inline __attribute__((always_inline))
	void sys_out32(uint32_t data, io_port_t port)
{
	__asm__ volatile("outl	%%eax, %%dx;\n\t" :  : "a"(data), "d"(port));
}


static inline inline __attribute__((always_inline))
	uint32_t sys_in32(io_port_t port)
{
	uint32_t ret;

	__asm__ volatile("inl	%%dx, %%eax;\n\t" : "=a"(ret) : "d"(port));
	return ret;
}

static inline inline __attribute__((always_inline))
	void sys_write8(uint8_t data, mm_reg_t addr)
{
	__asm__ volatile("movb	%0, %1;\n\t"
			 :
			 : "q"(data), "m" (*(volatile uint8_t *) addr)
			 : "memory");
}

static inline inline __attribute__((always_inline))
	uint8_t sys_read8(mm_reg_t addr)
{
	uint8_t ret;

	__asm__ volatile("movb	%1, %0;\n\t"
			 : "=q"(ret)
			 : "m" (*(volatile uint8_t *) addr)
			 : "memory");

	return ret;
}

static inline __attribute__((always_inline))
	void sys_write16(uint16_t data, mm_reg_t addr)
{
	__asm__ volatile("movw	%0, %1;\n\t"
			 :
			 : "r"(data), "m" (*(volatile uint16_t *) addr)
			 : "memory");
}

static inline inline __attribute__((always_inline))
	uint16_t sys_read16(mm_reg_t addr)
{
	uint16_t ret;

	__asm__ volatile("movw	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile uint16_t *) addr)
			 : "memory");

	return ret;
}

static inline inline __attribute__((always_inline))
	void sys_write32(uint32_t data, mm_reg_t addr)
{
	__asm__ volatile("movl	%0, %1;\n\t"
			 :
			 : "r"(data), "m" (*(volatile uint32_t *) addr)
			 : "memory");
}

static inline __attribute__((always_inline))
	uint32_t sys_read32(mm_reg_t addr)
{
	uint32_t ret;

	__asm__ volatile("movl	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile uint32_t *) addr)
			 : "memory");

	return ret;
}


#endif /* _ASMLANGUAGE */
#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */

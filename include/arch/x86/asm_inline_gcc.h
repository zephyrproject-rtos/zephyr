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
 * @brief Find first set bit searching from the LSB (inline)
 *
 * This routine finds the first bit set in the argument passed it and
 * returns the index of that bit.  Bits are numbered starting
 * at 1 from the least significant bit to 32 for the most significant bit.
 * A return value of zero indicates that the value passed is zero.
 *
 * @return bit position from 1 to 32, or 0 if the argument is zero.
 *
 * INTERNAL
 * For Intel64 (x86_64) architectures, the 'cmovzl' can be removed
 * and leverage the fact that the 'bsfl' doesn't modify the destination operand
 * when the source operand is zero.  The "bitpos" variable can be preloaded
 * into the destination register, and given the unconditional ++bitpos that
 * is performed after the 'cmovzl', the correct results are yielded.
 */

static ALWAYS_INLINE unsigned int find_first_set(unsigned int op)
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
 * @brief Find first set bit searching from the MSB (inline)
 *
 * This routine finds the first bit set in the argument passed it and
 * returns the index of that bit.  Bits are numbered starting
 * at 1 from the least significant bit to 32 for the most significant bit.
 * A return value of zero indicates that the value passed is zero.
 *
 * @return bit position from 1 to 32, or 0 if the argument is zero.
 *
 * INTERNAL
 * For Intel64 (x86_64) architectures, the 'cmovzl' can be removed
 * and leverage the fact that the 'bsfl' doesn't modify the destination operand
 * when the source operand is zero.  The "bitpos" variable can be preloaded
 * into the destination register, and given the unconditional ++bitpos that
 * is performed after the 'cmovzl', the correct results are yielded.
 */

static ALWAYS_INLINE unsigned int find_last_set(unsigned int op)
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

static inline inline __attribute__((always_inline))
	uint32_t _do_read_cpu_timestamp32(void)
{
	uint32_t rv;

	__asm__ volatile("rdtsc" : "=a"(rv) :  : "%edx");

	return rv;
}


/**
 *
 * @brief Output a byte to an IA-32 I/O port
 *
 * This function issues the 'out' instruction to write a byte to the specified
 * I/O port.
 *
 * @return N/A
 *
 * NOMANUAL
 */

static inline inline __attribute__((always_inline))
	void sys_out8(unsigned char data, unsigned int port)
{
	__asm__ volatile("outb	%%al, %%dx;\n\t" : : "a"(data), "d"(port));
}


/**
 *
 * @brief Input a byte from an IA-32 I/O port
 *
 * This function issues the 'in' instruction to read a byte from the specified
 * I/O port.
 *
 * @return the byte read from the specified I/O port
 *
 * NOMANUAL
 */

static inline inline __attribute__((always_inline))
	unsigned char sys_in8(unsigned int port)
{
	char retByte;

	__asm__ volatile("inb	%%dx, %%al;\n\t" : "=a"(retByte) : "d"(port));
	return retByte;
}


/**
 *
 * @brief Output a word to an IA-32 I/O port
 *
 * This function issues the 'out' instruction to write a word to the
 * specified I/O port.
 *
 * @return N/A
 *
 * NOMANUAL
 */

static inline inline __attribute__((always_inline))
	void sys_out16(unsigned short data, unsigned int port)
{
	__asm__ volatile("outw	%%ax, %%dx;\n\t" :  : "a"(data), "d"(port));
}


/**
 *
 * @brief Input a word from an IA-32 I/O port
 *
 * This function issues the 'in' instruction to read a word from the
 * specified I/O port.
 *
 * @return the word read from the specified I/O port
 *
 * NOMANUAL
 */

static inline inline __attribute__((always_inline))
	unsigned short sys_in16(unsigned int port)
{
	unsigned short retWord;

	__asm__ volatile("inw	%%dx, %%ax;\n\t" : "=a"(retWord) : "d"(port));
	return retWord;
}


/**
 *
 * @brief Output a long word to an IA-32 I/O port
 *
 * This function issues the 'out' instruction to write a long word to the
 * specified I/O port.
 *
 * @return N/A
 *
 * NOMANUAL
 */

static inline inline __attribute__((always_inline))
	void sys_out32(unsigned int data, unsigned int port)
{
	__asm__ volatile("outl	%%eax, %%dx;\n\t" :  : "a"(data), "d"(port));
}


/**
 *
 * @brief Input a long word from an IA-32 I/O port
 *
 * This function issues the 'in' instruction to read a long word from the
 * specified I/O port.
 *
 * @return the long read from the specified I/O port
 *
 * NOMANUAL
 */

static inline inline __attribute__((always_inline))
	unsigned long sys_in32(unsigned int port)
{
	unsigned long retLong;

	__asm__ volatile("inl	%%dx, %%eax;\n\t" : "=a"(retLong) : "d"(port));
	return retLong;
}

#endif /* _ASMLANGUAGE */
#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */

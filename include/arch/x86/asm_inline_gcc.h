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

/*******************************************************************************
*
* _do_irq_lock_inline - disable all interrupts on the CPU (inline)
*
* This routine disables interrupts.  It can be called from either interrupt,
* task or fiber level.  This routine returns an architecture-dependent
* lock-out key representing the "interrupt disable state" prior to the call;
* this key can be passed to irq_unlock_inline() to re-enable interrupts.
*
* The lock-out key should only be used as the argument to the
* irq_unlock_inline() API.  It should never be used to manually re-enable
* interrupts or to inspect or manipulate the contents of the source register.
*
* WARNINGS
* Invoking a VxMicro routine with interrupts locked may result in
* interrupts being re-enabled for an unspecified period of time.  If the
* called routine blocks, interrupts will be re-enabled while another
* context executes, or while the system is idle.
*
* The "interrupt disable state" is an attribute of a context.  Thus, if a
* fiber or task disables interrupts and subsequently invokes a VxMicro
* system routine that causes the calling context to block, the interrupt
* disable state will be restored when the context is later rescheduled
* for execution.
*
* RETURNS: An architecture-dependent lock-out key representing the
* "interrupt disable state" prior to the call.
*
* \NOMANUAL
*/

static inline __attribute__((always_inline))
	unsigned int _do_irq_lock_inline(void)
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


/*******************************************************************************
*
* _do_irq_unlock_inline - enable all interrupts on the CPU (inline)
*
* This routine can be called from either interrupt, task or fiber level.
* Invoked by kernel or by irq_unlock_inline()
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static inline __attribute__((always_inline))
	void _do_irq_unlock_inline(void)
{
	__asm__ volatile (
		"sti;\n\t"
		: :
		);
}


/*******************************************************************************
*
* find_first_set_inline - find first set bit searching from the LSB (inline)
*
* This routine finds the first bit set in the argument passed it and
* returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit to 32 for the most significant bit.
* A return value of zero indicates that the value passed is zero.
*
* RETURNS: bit position from 1 to 32, or 0 if the argument is zero.
*
* INTERNAL
* For Intel64 (x86_64) architectures, the 'cmovzl' can be removed
* and leverage the fact that the 'bsfl' doesn't modify the destination operand
* when the source operand is zero.  The "bitpos" variable can be preloaded
* into the destination register, and given the unconditional ++bitpos that
* is performed after the 'cmovzl', the correct results are yielded.
*/

static inline __attribute__((always_inline))
	unsigned int find_first_set_inline (unsigned int op)
{
	int bitpos;

	__asm__ volatile (

#if !defined(CONFIG_CMOV_UNSUPPORTED)

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

#endif /* !CONFIG_CMOV_UNSUPPORTED */
		);

	return (bitpos + 1);
}


/*******************************************************************************
*
* find_last_set_inline - find first set bit searching from the MSB (inline)
*
* This routine finds the first bit set in the argument passed it and
* returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit to 32 for the most significant bit.
* A return value of zero indicates that the value passed is zero.
*
* RETURNS: bit position from 1 to 32, or 0 if the argument is zero.
*
* INTERNAL
* For Intel64 (x86_64) architectures, the 'cmovzl' can be removed
* and leverage the fact that the 'bsfl' doesn't modify the destination operand
* when the source operand is zero.  The "bitpos" variable can be preloaded
* into the destination register, and given the unconditional ++bitpos that
* is performed after the 'cmovzl', the correct results are yielded.
*/

static inline inline __attribute__((always_inline))
	unsigned int find_last_set_inline (unsigned int op)
{
	int bitpos;

	__asm__ volatile (

#if !defined(CONFIG_CMOV_UNSUPPORTED)

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

#endif /* CONFIG_CMOV_UNSUPPORTED */
		);

	return (bitpos + 1);
}


/********************************************************
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


/*******************************************************************************
 *
 * _do_read_cpu_timestamp - get a 32 bit CPU timestamp counter
 *
 * RETURNS: a 32-bit number
 */

static inline inline __attribute__((always_inline))
	uint32_t _do_read_cpu_timestamp32(void)
{
	uint32_t rv;

	__asm__ volatile("rdtsc" : "=a"(rv) :  : "%edx");

	return rv;
}


/*******************************************************************************
*
* sys_out8 - output a byte to an IA-32 I/O port
*
* This function issues the 'out' instruction to write a byte to the specified
* I/O port.
*
* RETURNS: N/A
*
* NOMANUAL
*/

static inline inline __attribute__((always_inline))
	void sys_out8(unsigned char data, unsigned int port)
{
	__asm__ volatile("outb	%%al, %%dx;\n\t" : : "a"(data), "d"(port));
}


/*******************************************************************************
*
* sys_in8 - input a byte from an IA-32 I/O port
*
* This function issues the 'in' instruction to read a byte from the specified
* I/O port.
*
* RETURNS: the byte read from the specified I/O port
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


/*******************************************************************************
*
* sys_out16 - output a word to an IA-32 I/O port
*
* This function issues the 'out' instruction to write a word to the
* specified I/O port.
*
* RETURNS: N/A
*
* NOMANUAL
*/

static inline inline __attribute__((always_inline))
	void sys_out16(unsigned short data, unsigned int port)
{
	__asm__ volatile("outw	%%ax, %%dx;\n\t" :  : "a"(data), "d"(port));
}


/*******************************************************************************
*
* sys_in16 - input a word from an IA-32 I/O port
*
* This function issues the 'in' instruction to read a word from the
* specified I/O port.
*
* RETURNS: the word read from the specified I/O port
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


/*******************************************************************************
*
* sys_out32 - output a long word to an IA-32 I/O port
*
* This function issues the 'out' instruction to write a long word to the
* specified I/O port.
*
* RETURNS: N/A
*
* NOMANUAL
*/

static inline inline __attribute__((always_inline))
	void sys_out32(unsigned int data, unsigned int port)
{
	__asm__ volatile("outl	%%eax, %%dx;\n\t" :  : "a"(data), "d"(port));
}


/*******************************************************************************
*
* sys_in32 - input a long word from an IA-32 I/O port
*
* This function issues the 'in' instruction to read a long word from the
* specified I/O port.
*
* RETURNS: the long read from the specified I/O port
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

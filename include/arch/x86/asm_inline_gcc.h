/* Intel x86 GCC specific public inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
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
	__asm__ volatile("outb	%b0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static inline __attribute__((always_inline))
	uint8_t sys_in8(io_port_t port)
{
	uint8_t ret;

	__asm__ volatile("inb	%w1, %b0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static inline __attribute__((always_inline))
	void sys_out16(uint16_t data, io_port_t port)
{
	__asm__ volatile("outw	%w0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static inline __attribute__((always_inline))
	uint16_t sys_in16(io_port_t port)
{
	uint16_t ret;

	__asm__ volatile("inw	%w1, %w0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static inline __attribute__((always_inline))
	void sys_out32(uint32_t data, io_port_t port)
{
	__asm__ volatile("outl	%0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}


static inline __attribute__((always_inline))
	uint32_t sys_in32(io_port_t port)
{
	uint32_t ret;

	__asm__ volatile("inl	%w1, %0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}


static inline __attribute__((always_inline))
	void sys_io_set_bit(io_port_t port, int bit)
{
	uint32_t reg = 0;

	__asm__ volatile("inl	%%dx, %%eax\n"
			 "mov	%1, 1\n"
			 "shl	%%cl, %1\n"
			 "or	%%eax, %1\n"
			 "outl	%%eax, %%dx;\n\t"
			 :
			 : "d" (port),
			   "r" (reg), "d" (bit)
			 : "memory", "cc");
}

static inline __attribute__((always_inline))
	void sys_io_clear_bit(io_port_t port, int bit)
{
	uint32_t reg = 0;

	__asm__ volatile("inl	%%dx, %%eax\n"
			 "mov	%1, 1\n"
			 "shl	%%cl, %1\n"
			 "and	%%eax, %1\n"
			 "outl	%%eax, %%dx;\n\t"
			 :
			 : "d" (port),
			   "r" (reg), "d" (bit)
			 : "memory", "cc");
}

static inline __attribute__((always_inline))
	int sys_io_test_bit(io_port_t port, int bit)
{
	uint32_t ret;

	__asm__ volatile("inl	%%dx, %%eax\n"
			 "bt	%2, %%eax\n"
			 "lahf\n"
			 "mov	%1, %%eax\n"
			 "clc;\n\t"
			 : "=r" (ret)
			 : "d" (port),
			   "Mr" (bit)
			 : "memory", "cc");

	return (ret & 1);
}

static inline __attribute__((always_inline))
	int sys_io_test_and_set_bit(io_port_t port, int bit)
{
	int ret;

	ret = sys_io_test_bit(port, bit);
	sys_io_set_bit(port, bit);

	return ret;
}

static inline __attribute__((always_inline))
	int sys_io_test_and_clear_bit(io_port_t port, int bit)
{
	int ret;

	ret = sys_io_test_bit(port, bit);
	sys_io_clear_bit(port, bit);

	return ret;
}

static inline __attribute__((always_inline))
	void sys_write8(uint8_t data, mm_reg_t addr)
{
	__asm__ volatile("movb	%0, %1;\n\t"
			 :
			 : "q"(data), "m" (*(volatile uint8_t *) addr)
			 : "memory");
}

static inline __attribute__((always_inline))
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

static inline __attribute__((always_inline))
	uint16_t sys_read16(mm_reg_t addr)
{
	uint16_t ret;

	__asm__ volatile("movw	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile uint16_t *) addr)
			 : "memory");

	return ret;
}

static inline __attribute__((always_inline))
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


static inline __attribute__((always_inline))
	void sys_set_bit(mem_addr_t addr, int bit)
{
	__asm__ volatile("btsl	%1, %0;\n\t"
			 : "+m" (*(volatile uint32_t *) (addr))
			 : "Ir" (bit)
			 : "memory");
}

static inline __attribute__((always_inline))
	void sys_clear_bit(mem_addr_t addr, int bit)
{
	__asm__ volatile("btrl	%1, %0;\n\t"
			 : "+m" (*(volatile uint32_t *) (addr))
			 : "Ir" (bit));
}

static inline __attribute__((always_inline))
	int sys_test_bit(mem_addr_t addr, int bit)
{
	int ret;

	__asm__ volatile("btl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile uint32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

static inline __attribute__((always_inline))
	int sys_test_and_set_bit(mem_addr_t addr, int bit)
{
	int ret;

	__asm__ volatile("btsl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile uint32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

static inline __attribute__((always_inline))
	int sys_test_and_clear_bit(mem_addr_t addr, int bit)
{
	int ret;

	__asm__ volatile("btrl	%2, %1;\n\t"
			 "sbb	%0, %0\n\t"
			 : "=r" (ret), "+m" (*(volatile uint32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

#endif /* _ASMLANGUAGE */
#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */

/* Intel x86 GCC specific public inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_X86_ASM_INLINE_GCC_H_

/*
 * The file must not be included directly
 * Include kernel.h instead
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
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

static ALWAYS_INLINE unsigned int _do_irq_lock(void)
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

static ALWAYS_INLINE void z_do_irq_unlock(void)
{
	__asm__ volatile (
		"sti;\n\t"
		: : : "memory"
		);
}


/**
 *  @brief read timestamp register ensuring serialization
 */

static inline u64_t z_tsc_read(void)
{
	union {
		struct  {
			u32_t lo;
			u32_t hi;
		};
		u64_t  value;
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

static ALWAYS_INLINE
	u32_t z_do_read_cpu_timestamp32(void)
{
	u32_t rv;

	__asm__ volatile("rdtsc" : "=a"(rv) :  : "%edx");

	return rv;
}



#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ASM_INLINE_GCC_H_ */

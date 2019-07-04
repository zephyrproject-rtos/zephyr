/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_

#include <generated_dts_board.h>
#include <stdbool.h>
#include <irq.h>

#ifdef CONFIG_X86_LONGMODE
#include <arch/x86/intel64/arch.h>
#else
#include <arch/x86/ia32/arch.h>
#endif

#include <arch/common/ffs.h>

#ifndef _ASMLANGUAGE

extern void z_arch_irq_enable(unsigned int irq);
extern void z_arch_irq_disable(unsigned int irq);

extern u32_t z_timer_cycle_get_32(void);
#define z_arch_k_cycle_get_32() z_timer_cycle_get_32()

/**
 * Returns true if interrupts were unlocked prior to the
 * z_arch_irq_lock() call that produced the key argument.
 */
static ALWAYS_INLINE bool z_arch_irq_unlocked(unsigned int key)
{
	return (key & 0x200) != 0;
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

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_ */

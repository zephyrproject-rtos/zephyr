/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _X86_64_ARCH_H
#define _X86_64_ARCH_H

#include <arch/common/sys_io.h>
#include <arch/common/ffs.h>

#define STACK_ALIGN 8

#define DT_INST_0_INTEL_HPET_BASE_ADDRESS	0xFED00000U
#define DT_INST_0_INTEL_HPET_IRQ_0		2
#define DT_INST_0_INTEL_HPET_IRQ_0_PRIORITY	4

typedef struct z_arch_esf_t z_arch_esf_t;

static inline u32_t z_arch_k_cycle_get_32(void)
{
#ifdef CONFIG_HPET_TIMER
	extern u32_t z_timer_cycle_get_32(void);
	return z_timer_cycle_get_32();
#else
	return (u32_t)z_arch_k_cycle_get_64();
#endif
}

/* Not a standard Zephyr function, but probably will be */
static inline unsigned long long z_arch_k_cycle_get_64(void)
{
	unsigned int hi, lo;

	__asm__ volatile("rdtsc" : "=d"(hi), "=a"(lo));
	return (((unsigned long long)hi) << 32) | lo;
}

static inline unsigned int z_arch_irq_lock(void)
{
	unsigned long long key;

	__asm__ volatile("pushfq; cli; popq %0" : "=r"(key));
	return (int)key;
}

static inline void z_arch_irq_unlock(unsigned int key)
{
	if (key & 0x200) {
		__asm__ volatile("sti");
	}
}

/**
 * Returns true if interrupts were unlocked prior to the
 * z_arch_irq_lock() call that produced the key argument.
 */
static inline bool z_arch_irq_unlocked(unsigned int key)
{
	return (key & 0x200) != 0;
}

void z_arch_irq_enable(unsigned int irq);

void z_arch_irq_disable(unsigned int irq);

#define Z_ARCH_IRQ_CONNECT(irq, pri, isr, arg, flags) \
	z_arch_irq_connect_dynamic(irq, pri, isr, arg, flags)

extern int x86_64_except_reason;


/* Vector 5 is the "bounds" exception which is otherwise vestigial
 * (BOUND is an illegal instruction in long mode)
 */
#define Z_ARCH_EXCEPT(reason) do {		\
		x86_64_except_reason = reason;	\
		__asm__ volatile("int $5");	\
	} while (false)


#endif /* _X86_64_ARCH_H */

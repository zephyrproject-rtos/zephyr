/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _KERNEL_ARCH_FUNC_H
#define _KERNEL_ARCH_FUNC_H

#include <irq.h>
#include <xuk-switch.h>

static inline void kernel_arch_init(void)
{
	/* This is a noop, we already took care of things before
	 * _Cstart() is entered
	 */
}

static inline struct _cpu *_arch_curr_cpu(void)
{
	long long ret, off = 0;

	/* The struct _cpu pointer for the current CPU lives at the
	 * start of the the FS segment
	 */
	__asm__("movq %%fs:(%1), %0" : "=r"(ret) : "r"(off));
	return (struct _cpu *)(long)ret;
}

static inline unsigned int _arch_irq_lock(void)
{
	unsigned long long key;

	__asm__ volatile("pushfq; cli; popq %0" : "=r"(key));
	return (int)key;
}

static inline void _arch_irq_unlock(unsigned int key)
{
	if (key & 0x200) {
		__asm__ volatile("sti");
	}
}

static inline void arch_nop(void)
{
	__asm__ volatile("nop");
}

void _arch_irq_disable(unsigned int irq);
void _arch_irq_enable(unsigned int irq);

/* Not a standard Zephyr function, but probably will be */
static inline unsigned long long _arch_k_cycle_get_64(void)
{
	unsigned int hi, lo;

	__asm__ volatile("rdtsc" : "=d"(hi), "=a"(lo));
	return (((unsigned long long)hi) << 32) | lo;
}

static inline unsigned int _arch_k_cycle_get_32(void)
{
#ifdef CONFIG_HPET_TIMER
	extern u32_t _timer_cycle_get_32(void);
	return _timer_cycle_get_32();
#else
	return (u32_t)_arch_k_cycle_get_64();
#endif
}

#define _is_in_isr() (_arch_curr_cpu()->nested != 0)

static inline void _arch_switch(void *switch_to, void **switched_from)
{
	xuk_switch(switch_to, switched_from);
}

static inline u32_t x86_apic_scaled_tsc(void)
{
	u32_t lo, hi;
	u64_t tsc;

	__asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
	tsc = (((u64_t)hi) << 32) | lo;
	return (u32_t)(tsc >> CONFIG_XUK_APIC_TSC_SHIFT);
}

void x86_apic_set_timeout(u32_t cyc_from_now);

#define _ARCH_IRQ_CONNECT(irq, pri, isr, arg, flags) \
	_arch_irq_connect_dynamic(irq, pri, isr, arg, flags)

extern int x86_64_except_reason;


/* Vector 5 is the "bounds" exception which is otherwise vestigial
 * (BOUND is an illegal instruction in long mode)
 */
#define _ARCH_EXCEPT(reason) do {		\
		x86_64_except_reason = reason;	\
		__asm__ volatile("int $5");	\
	} while (false)

#endif /* _KERNEL_ARCH_FUNC_H */

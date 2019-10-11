/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _KERNEL_ARCH_FUNC_H
#define _KERNEL_ARCH_FUNC_H

#include <irq.h>
#include <xuk-switch.h>

static inline void z_arch_kernel_init(void)
{
	/* This is a noop, we already took care of things before
	 * z_cstart() is entered
	 */
}

static inline struct _cpu *z_arch_curr_cpu(void)
{
	long long ret, off = 0;

	/* The struct _cpu pointer for the current CPU lives at the
	 * start of the the FS segment
	 */
	__asm__("movq %%fs:(%1), %0" : "=r"(ret) : "r"(off));
	return (struct _cpu *)(long)ret;
}

static inline void z_arch_nop(void)
{
	__asm__ volatile("nop");
}

static inline bool z_arch_is_in_isr(void)
{
	return z_arch_curr_cpu()->nested != 0U;
}

static inline u32_t x86_apic_scaled_tsc(void)
{
	u32_t lo, hi;
	u64_t tsc;

	__asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
	tsc = (((u64_t)hi) << 32) | lo;
	return (u32_t)(tsc >> CONFIG_XUK_APIC_TSC_SHIFT);
}

static inline void z_arch_switch(void *switch_to, void **switched_from)
{
	xuk_switch(switch_to, switched_from);
}

void x86_apic_set_timeout(u32_t cyc_from_now);

void z_arch_sched_ipi(void);

#endif /* _KERNEL_ARCH_FUNC_H */

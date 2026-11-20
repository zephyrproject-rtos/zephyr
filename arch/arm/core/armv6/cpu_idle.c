/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARMv6 (ARM1176) power management. ARM1176 has no dsb/wfi instructions;
 * the equivalents are CP15 c7 operations. IRQ enable/disable use cpsie/cpsid.
 */

#include <zephyr/kernel.h>
#include <zephyr/tracing/tracing.h>

/* CP15 c7 maintenance ops (value in Rt is ignored). */
#define ARMV6_DSB() __asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0) : "memory")
#define ARMV6_WFI() __asm__ volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) : "memory")

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	/* Drain memory transactions, then wait for an interrupt. */
	ARMV6_DSB();
	ARMV6_WFI();

	/* Re-enable interrupts to service the wake-up source. */
	__asm__ volatile("cpsie i" : : : "memory");
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	__asm__ volatile("cpsid i" : : : "memory");

	ARMV6_DSB();
	ARMV6_WFI();

	if (!key) {
		__asm__ volatile("cpsie i" : : : "memory");
	}
}
#endif

/**
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/arch/cpu.h>
#include <xc.h>

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	/* Mask all interrupts using DISI */
	__asm__ volatile("DISICTL #7\n\t");
	Idle();
	/* Unmask interrupts (clear DISI) */
	__asm__ volatile("DISICTL #0\n\t");
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	/* Mask all interrupts using DISI */
	__asm__ volatile("DISICTL #7\n\t");
	Idle();
	arch_irq_unlock(key);
	/* Unmask interrupts (clear DISI) */
	__asm__ volatile("DISICTL #0\n\t");
}
#endif

FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	(void)reason;
	CODE_UNREACHABLE;
}

/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/asm_inline_gcc.h>

#ifdef CONFIG_TRACING
#include <zephyr/tracing/tracing.h>
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
#ifdef CONFIG_TRACING
	sys_trace_idle();
#endif /* CONFIG_TRACING */

	__disable_irq();

	/* drain write buffer */
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" :: "r"(0) : "memory");

	/* wait for interrupt */
	__asm__ volatile("mcr p15, 0, %0, c7, c0, 4" :: "r"(0) : "memory");

	__enable_irq();

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
#ifdef CONFIG_TRACING
	sys_trace_idle();
#endif /* CONFIG_TRACING */

	__disable_irq();

	arch_irq_unlock(key);

	__enable_irq();

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
}
#endif

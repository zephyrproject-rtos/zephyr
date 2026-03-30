/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2026 Synaptics Incorporated
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARM64 Cortex-A power management
 */

#include <zephyr/kernel.h>
#include <zephyr/tracing/tracing.h>

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif

	__asm__ volatile("dsb sy" : : : "memory");
	wfi();

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif

	enable_irq();
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif

	disable_irq();

	__asm__ volatile("isb" : : : "memory");
	wfe();

#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif

	if (!(key & DAIF_IRQ_BIT)) {
		enable_irq();
	}
}
#endif

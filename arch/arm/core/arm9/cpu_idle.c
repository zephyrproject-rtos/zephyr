/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 idle handling
 */

#include <zephyr/kernel.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/arch/arm/arm9/lib_helpers.h>

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	__DSB();
	__WFI();

	__enable_irq();
	__ISB();
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	__disable_irq();

	__DSB();
	__WFI();

	if (!key) {
		__enable_irq();
	}
}
#endif

/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	__asm__ volatile("wfi");
#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
	irq_unlock(MSTATUS_IEN);
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	__asm__ volatile("wfi");
#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
	irq_unlock(key);
}
#endif

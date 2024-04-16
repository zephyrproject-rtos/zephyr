/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

#ifdef CONFIG_SOC_SERIES_NSIM_RV_RMX500

void __weak arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock(MSTATUS_IEN);
	__asm__ volatile("wfi");
}

void __weak arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);
	__asm__ volatile("wfi");
}

#else
void __weak arch_cpu_idle(void)
{
	sys_trace_idle();
	__asm__ volatile("wfi");
	irq_unlock(MSTATUS_IEN);
}

void __weak arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	__asm__ volatile("wfi");
	irq_unlock(key);
}
#endif /* SOC_SERIES_NSIM_RV_RMX500 */

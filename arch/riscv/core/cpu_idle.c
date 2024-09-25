/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

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

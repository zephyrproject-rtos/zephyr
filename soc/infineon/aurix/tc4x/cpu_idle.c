/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

void __weak arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock((1 << 15));

	__asm volatile ("wait");
}

void __weak arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);

	__asm volatile ("wait");
}

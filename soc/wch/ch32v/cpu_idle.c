/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

void arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock(MSTATUS_IEN);
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);
}

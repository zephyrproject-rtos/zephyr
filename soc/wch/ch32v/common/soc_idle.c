/*
 * Copyright (c) 2025 James Bennion-Pedley
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

/* note: default risc-v idle cannot be used as this will break ongoing DMA transactions */
void arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock(MSTATUS_IEN);
	sys_trace_idle_exit();
}

/* note: default risc-v idle cannot be used as this will break ongoing DMA transactions */
void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);
	sys_trace_idle_exit();
}

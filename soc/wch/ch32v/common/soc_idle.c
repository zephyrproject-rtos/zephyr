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
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	irq_unlock(MSTATUS_IEN);
#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
}

/* note: default risc-v idle cannot be used as this will break ongoing DMA transactions */
void arch_cpu_atomic_idle(unsigned int key)
{
#if defined(CONFIG_TRACING)
	sys_trace_idle();
#endif
	irq_unlock(key);
#if defined(CONFIG_TRACING)
	sys_trace_idle_exit();
#endif
}

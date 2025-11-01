/*
 * Copyright (C) 2025 Michael Hope <michaelh@juju.nz>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

void arch_cpu_idle(void)
{
	/*
	 * The RISC-V Machine-Level ISA section 3.3.3 says that `wfi` will complete even if
	 * interrupts are masked, but the QingKe V2A does not do this. Work-around by enabling
	 * interrupts first.
	 */
	sys_trace_idle();
	irq_unlock(MSTATUS_IEN);
	__asm__ volatile("wfi");
	sys_trace_idle_exit();
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);
	__asm__ volatile("wfi");
	sys_trace_idle_exit();
}

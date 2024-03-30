/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

/*
 * Due to a HW issue, VPR requires MSTATUS.MIE to be enabled when entering sleep.
 * Otherwise it would not wake up.
 */
void arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock(MSTATUS_IEN);
	__asm__ volatile("wfi");
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(MSTATUS_IEN);
	__asm__ volatile("wfi");

	/* Disable interrupts if needed. */
	__asm__ volatile ("csrc mstatus, %0"
			  :
			  : "r" (~key & MSTATUS_IEN)
			  : "memory");
}

/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/tracing/tracing.h>

/* A pending interrupt releases HALT also while PSW.ID = 1 */
void arch_cpu_idle(void)
{
	sys_trace_idle();
	__asm__ volatile("di\n\thalt\n\tei" ::: "memory");
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	__asm__ volatile("di\n\thalt" ::: "memory");
	arch_irq_unlock(key);
}

/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <tracing/tracing.h>

static void leon_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);

	__asm__ volatile ("wr  %g0, %asr19");
}

void arch_cpu_idle(void)
{
	leon_idle(0);
}

void arch_cpu_atomic_idle(unsigned int key)
{
	leon_idle(key);
}

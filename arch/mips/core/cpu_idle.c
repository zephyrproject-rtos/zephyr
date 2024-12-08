/*
 * Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>

#include <zephyr/tracing/tracing.h>

static ALWAYS_INLINE void mips_idle(unsigned int key)
{
	sys_trace_idle();

	/* unlock interrupts */
	irq_unlock(key);

	/* wait for interrupt */
	__asm__ volatile("wait");
}

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	mips_idle(1);
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	mips_idle(key);
}
#endif

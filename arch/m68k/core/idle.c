/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

static ALWAYS_INLINE void m68k_idle(void)
{
	__asm__ volatile("stop #0x2000" ::: "memory");
}

void arch_cpu_idle(void)
{
	m68k_idle();
}

void arch_cpu_atomic_idle(unsigned int key)
{
	m68k_idle();
	arch_irq_unlock(key);
}

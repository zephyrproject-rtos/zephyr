/**
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/arch/cpu.h>
#include <xc.h>

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	__builtin_disable_interrupts();
	Idle();
	__builtin_enable_interrupts();
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	__builtin_disable_interrupts();
	Idle();
	arch_irq_unlock(key);
	__builtin_enable_interrupts();
}
#endif

FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	(void)reason;
	CODE_UNREACHABLE;
}

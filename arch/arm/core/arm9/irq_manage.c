/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 interrupt management
 *
 * ARMv5 cores only distinguish IRQ from FIQ; every other aspect of interrupt
 * control lives in the SoC's interrupt controller, reached through the
 * z_soc_irq_*() interface.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/pm/pm.h>

void z_arm_fatal_error(unsigned int reason, const struct arch_esf *esf);

/**
 * @brief Spurious interrupt handler
 *
 * Installed in all _sw_isr_table slots at boot time.
 */
void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	z_arm_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

#ifdef CONFIG_PM
void _arch_isr_direct_pm(void)
{
	unsigned int key;

	key = irq_lock();

	if (_kernel.idle) {
		_kernel.idle = 0;
		pm_system_resume();
	}

	irq_unlock(key);
}
#endif

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_arm_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

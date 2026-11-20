/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/intc_renesas_rh850.h>
#include <kernel_arch_func.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

void arch_irq_enable(unsigned int irq)
{
	intc_rh850_irq_enable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	intc_rh850_irq_disable(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return intc_rh850_irq_is_enabled(irq) ? 1 : 0;
}

void z_rh850_irq_config(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

	intc_rh850_irq_set_priority(irq, prio);
}

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);

	LOG_ERR("Spurious interrupt, EIIC %#x", rh850_sysreg_read(RH850_SR_EIIC));
	z_fatal_error(K_ERR_SPURIOUS_IRQ, arch_curr_cpu()->arch.esf);
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_rh850_irq_config(irq, priority, flags);

	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

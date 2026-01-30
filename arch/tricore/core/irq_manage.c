/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/interrupt_controller/intc_aurix_ir.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

void arch_irq_enable(unsigned int irq)
{
	intc_aurix_ir_irq_enable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	intc_aurix_ir_irq_disable(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return intc_aurix_ir_irq_is_enabled(irq);
}

void z_tricore_irq_config(unsigned int irq, unsigned int prio, unsigned int flags)
{
	intc_aurix_ir_irq_config(irq, prio, flags);
}

FUNC_NORETURN void z_irq_spurious(const void *unused)
{
#ifdef CONFIG_EMPTY_IRQ_SPURIOUS
	while (1) {
	}

	CODE_UNREACHABLE;
#else
	unsigned int irq = intc_aurix_ir_get_active();

	LOG_ERR("Spurious interrupt detected! IRQ: %d", irq);

	extern void z_tricore_fatal_error(unsigned int reason, const struct arch_esf *lower);
	z_tricore_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
	CODE_UNREACHABLE;
#endif /* CONFIG_EMPTY_IRQ_SPURIOUS */
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags)
{
	z_isr_install(irq, routine, parameter);
	intc_aurix_ir_irq_config(irq, priority, flags);
	return irq;
}

#ifdef CONFIG_SHARED_INTERRUPTS
int arch_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
				void (*routine)(const void *parameter), const void *parameter,
				uint32_t flags)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(flags);

	return z_isr_uninstall(irq, routine, parameter);
}
#endif /* CONFIG_SHARED_INTERRUPTS */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

#ifdef CONFIG_PM
void arch_isr_direct_pm(void)
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

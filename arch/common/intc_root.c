/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Architecture interrupt control functions implemented on top of the root
 * interrupt controller device. Used by every platform whose root controller
 * is an interrupt controller driver, in place of per-platform glue.
 */

#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/interrupt_controller/intc.h>

void arch_irq_enable(unsigned int irq)
{
	intc_irq_enable(intc_root_device, irq);
}

void arch_irq_disable(unsigned int irq)
{
	intc_irq_disable(intc_root_device, irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return intc_irq_is_enabled(intc_root_device, irq);
}

void intc_root_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	intc_irq_priority_set(intc_root_device, irq, prio, flags);
}

#if defined(CONFIG_ARCH_HAS_IRQ_PENDING_OPS)
void arch_irq_set_pending(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "IRQ %u out of range", irq);

	intc_irq_set_pending(intc_root_device, irq);
}

void arch_irq_clear_pending(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "IRQ %u out of range", irq);

	intc_irq_clear_pending(intc_root_device, irq);
}

bool arch_irq_is_pending(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "IRQ %u out of range", irq);

	return intc_irq_is_pending(intc_root_device, irq);
}
#endif

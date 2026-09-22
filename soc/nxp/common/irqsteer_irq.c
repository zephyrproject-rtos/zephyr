/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Platform implementation of the SoC multi-level interrupt control
 * functions for i.MX platforms where the IRQSTEER aggregator is the
 * second-level interrupt controller. Interrupt controller drivers only
 * expose their own namespaced API; mapping z_soc_irq_* is the
 * platform's responsibility.
 */

#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/nxp_irqsteer.h>

void z_soc_irq_enable(uint32_t irq)
{
	nxp_irqstr_irq_enable(irq);
}

void z_soc_irq_disable(uint32_t irq)
{
	nxp_irqstr_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return nxp_irqstr_irq_is_enabled(irq);
}

#if defined(CONFIG_ARM)
void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	nxp_irqstr_irq_priority_set(irq, prio, flags);
}
#endif

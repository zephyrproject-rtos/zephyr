/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Platform implementation of the architecture interrupt control
 * functions, backed by the Hazard3 interrupt controller driver.
 * Interrupt controller drivers only expose their own namespaced API;
 * mapping arch_irq_* to the root interrupt controller is the
 * platform's responsibility.
 */

#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/intc_hazard3.h>

void arch_irq_enable(unsigned int irq)
{
	intc_hazard3_irq_enable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	intc_hazard3_irq_disable(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return intc_hazard3_irq_is_enabled(irq);
}

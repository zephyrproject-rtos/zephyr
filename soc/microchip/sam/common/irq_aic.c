/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/interrupt_controller/intc_mchp_aic_g1.h>

unsigned int z_soc_irq_get_active(void)
{
	return z_aic_irq_get_active();
}

void z_soc_irq_eoi(unsigned int irq)
{
	z_aic_irq_eoi(irq);
}

void z_soc_irq_init(void)
{
	z_aic_irq_init();
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	/* Configure interrupt type and priority */
	z_aic_irq_priority_set(irq, prio, flags);
}

void z_soc_irq_enable(unsigned int irq)
{
	z_aic_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	z_aic_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	/* Check if interrupt is enabled */
	return z_aic_irq_is_enabled(irq);
}

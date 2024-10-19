/*
 * Copyright (c) 2024 Abe Kohandel <abe.kohandel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "drivers/interrupt_controller/intc_ti_am335x.h"

void z_soc_irq_init(void)
{
	intc_ti_am335x_irq_init();
}

void z_soc_irq_enable(unsigned int irq)
{
	intc_ti_am335x_irq_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	intc_ti_am335x_irq_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return intc_ti_am335x_irq_is_enabled(irq);
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	intc_ti_am335x_irq_priority_set(irq, prio, flags);
}

unsigned int z_soc_irq_get_active(void)
{
	return intc_ti_am335x_irq_get_active();
}

void z_soc_irq_eoi(unsigned int irq)
{
	return intc_ti_am335x_irq_eoi(irq);
}

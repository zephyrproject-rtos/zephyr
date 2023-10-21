/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/intc_ch32v_pfic.h>

void z_soc_irq_enable(unsigned int irq)
{
	ch32v_pfic_enable(irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	ch32v_pfic_disable(irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return ch32v_pfic_is_enabled(irq);
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	ch32v_pfic_priority_set(irq, prio, flags);
}

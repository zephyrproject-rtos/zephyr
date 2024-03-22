/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal/nrf_vpr_clic.h>

void arch_irq_enable(unsigned int irq)
{
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, irq, true);
}

void arch_irq_disable(unsigned int irq)
{
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, irq, false);
}

void arch_irq_priority_set(unsigned int irq, unsigned int prio)
{
	nrf_vpr_clic_int_priority_set(NRF_VPRCLIC, irq, prio);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return nrf_vpr_clic_int_enable_check(NRF_VPRCLIC, irq);
}

/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <platform/nrf_802154_irq.h>

#include <zephyr/irq.h>
#include <nrfx.h>

void nrf_802154_irq_init(uint32_t irqn, int32_t prio, nrf_802154_isr_t isr)
{
	uint32_t flags = 0U;

	if (prio < 0) {
		prio = 0;
		flags |= IRQ_ZERO_LATENCY;
	}

	irq_connect_dynamic(irqn, prio, isr, NULL, flags);
}

void nrf_802154_irq_enable(uint32_t irqn)
{
	irq_enable(irqn);
}

void nrf_802154_irq_disable(uint32_t irqn)
{
	irq_disable(irqn);
}

void nrf_802154_irq_set_pending(uint32_t irqn)
{
	/* Zephyr does not provide abstraction layer for setting pending IRQ */
	NVIC_SetPendingIRQ(irqn);
}

void nrf_802154_irq_clear_pending(uint32_t irqn)
{
	/* Zephyr does not provide abstraction layer for clearing pending IRQ */
	NVIC_ClearPendingIRQ(irqn);
}

bool nrf_802154_irq_is_enabled(uint32_t irqn)
{
	return irq_is_enabled(irqn);
}

uint32_t nrf_802154_irq_priority_get(uint32_t irqn)
{
	return NVIC_GetPriority(irqn);
}

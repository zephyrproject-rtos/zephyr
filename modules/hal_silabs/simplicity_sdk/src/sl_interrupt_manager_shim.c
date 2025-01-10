/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shim for sl_interrupt_manager API using Zephyr API
 */

#include <zephyr/kernel.h>

#include "sl_interrupt_manager.h"

#define LOCK_KEY_DEFAULT 0xFFFFFFFFU

static unsigned int lock_key = LOCK_KEY_DEFAULT;

void sl_interrupt_manager_disable_interrupts(void)
{
	__ASSERT(lock_key == LOCK_KEY_DEFAULT, "Interrupt manager doesn't support nested disable");
	lock_key = irq_lock();
}

void sl_interrupt_manager_enable_interrupts(void)
{
	irq_unlock(lock_key);
	lock_key = LOCK_KEY_DEFAULT;
}

void sl_interrupt_manager_disable_irq(int32_t irqn)
{
	irq_disable(irqn);
}

void sl_interrupt_manager_enable_irq(int32_t irqn)
{
	irq_enable(irqn);
}

bool sl_interrupt_manager_is_irq_disabled(int32_t irqn)
{
	return !irq_is_enabled(irqn);
}

bool sl_interrupt_manager_is_irq_pending(int32_t irqn)
{
	return NVIC_GetPendingIRQ(irqn);
}

void sl_interrupt_manager_set_irq_pending(int32_t irqn)
{
	NVIC_SetPendingIRQ(irqn);
}

void sl_interrupt_manager_clear_irq_pending(int32_t irqn)
{
	NVIC_ClearPendingIRQ(irqn);
}

uint32_t sl_interrupt_manager_get_irq_priority(int32_t irqn)
{
	return NVIC_GetPriority(irqn);
}

void sl_interrupt_manager_set_irq_priority(int32_t irqn, uint32_t priority)
{
	NVIC_SetPriority(irqn, priority);
}

uint32_t sl_interrupt_manager_get_active_irq(int32_t irqn)
{
	return NVIC_GetActive(irqn);
}

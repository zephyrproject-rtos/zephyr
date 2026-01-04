/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <zephyr/irq.h>
#include <zephyr/irq_offload.h>
#include <xc.h>

static irq_offload_routine_t _offload_routine;
static const void *offload_param;

void z_irq_do_offload(void)
{
	irq_offload_routine_t tmp;

	if (_offload_routine != NULL) {

		tmp = _offload_routine;
		_offload_routine = NULL;

		tmp((const void *)offload_param);
	}
}

void handler(void)
{
	z_irq_do_offload();
}

void arch_irq_offload_init(void)
{
	IRQ_CONNECT(CONFIG_DSPIC33_IRQ_OFFLOAD_IRQ, 1, handler, NULL, 0);
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	uint32_t key = irq_lock();

	_offload_routine = routine;
	offload_param = parameter;
	irq_enable(CONFIG_DSPIC33_IRQ_OFFLOAD_IRQ);
	z_dspic_enter_irq(CONFIG_DSPIC33_IRQ_OFFLOAD_IRQ);
	irq_unlock(key);
}

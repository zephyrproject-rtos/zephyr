/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software interrupts utility code - Renesas rx architecture implementation.
 *
 * The code is using the first software interrupt (SWINT) of the RX processor
 * should this interrupt ever be used for something else, this has to be
 * changed - maybe to the second software interrupt (SWINT2).
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/util.h>

#define SWINT1_NODE            DT_NODELABEL(swint1)
#define SWINT1_IRQ_LINE        DT_IRQN(SWINT1_NODE)
#define SWINT1_PRIO            DT_IRQ(SWINT1_NODE, priority)
/* Address of the software interrupt trigger register for SWINT1 */
#define SWINT_REGISTER_ADDRESS DT_REG_ADDR(SWINT1_NODE)
#define SWINTR_SWINT           *(uint8_t *)(SWINT_REGISTER_ADDRESS)

static irq_offload_routine_t _offload_routine;
static const void *offload_param;

void z_irq_do_offload(void)
{
	irq_offload_routine_t tmp;

	if (!_offload_routine) {
		return;
	}

	tmp = _offload_routine;
	_offload_routine = NULL;

	tmp((const void *)offload_param);
}

static void swi0_handler(void)
{
	z_irq_do_offload();
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	_offload_routine = routine;
	offload_param = parameter;

	SWINTR_SWINT = 1;
}

void arch_irq_offload_init(void)
{
	IRQ_CONNECT(SWINT1_IRQ_LINE, SWINT1_PRIO, swi0_handler, NULL, 0);
	irq_enable(SWINT1_IRQ_LINE);
}

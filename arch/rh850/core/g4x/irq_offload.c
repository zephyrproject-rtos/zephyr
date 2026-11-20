/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software interrupts utility code - Renesas rh850 architecture implementation.
 *
 * The code is using the first software interrupt (SWINT0) of the RH850 processor
 * should this interrupt ever be used for something else, this has to be
 * changed to other software interrupt (SWINT0-SWINT3).
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/util.h>

#define SWINT0_NODE            DT_NODELABEL(swint0)
#define SWINT0_IRQ_LINE        DT_IRQN(SWINT0_NODE)
#define SWINT0_PRIO            DT_IRQ(SWINT0_NODE, priority)
/* Address of the software interrupt trigger register for SWINT0 */
#define SWINT_REGISTER_ADDRESS DT_REG_ADDR(SWINT0_NODE)

static irq_offload_routine_t volatile _offload_routine;
static const void *volatile offload_param;

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

static void swi0_handler(const void *unused)
{
	ARG_UNUSED(unused);

	/* Clear SW trigger interrupt */
	sys_write8(0U, SWINT_REGISTER_ADDRESS);

	/* Call call-back function */
	z_irq_do_offload();
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	__ASSERT(!k_is_in_isr(), "nested irq_offload is not supported");

	offload_param = parameter;
	_offload_routine = routine;

	sys_write8(1U, SWINT_REGISTER_ADDRESS);

	/* Make sure the trigger reached the interrupt controller */
	(void)sys_read8(SWINT_REGISTER_ADDRESS);
	__asm__ volatile("syncp" ::: "memory");

	/* irq_offload() is synchronous: wait until the handler has run */
	while (_offload_routine != NULL) {
	}
}

void arch_irq_offload_init(void)
{
	IRQ_CONNECT(SWINT0_IRQ_LINE, SWINT0_PRIO, swi0_handler, NULL, 0);
	irq_enable(SWINT0_IRQ_LINE);
}

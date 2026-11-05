/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software interrupts utility code - Renesas rh850 architecture implementation.
 *
 * The code uses the PE0 interrupt of the enabled renesas,rh850-swint
 * devicetree node. If this software interrupt is ever used for something
 * else, irq_offload has to use another software interrupt.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/util.h>

#define SWINT_NODE             DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_rh850_swint)
#define SWINT_IRQ_LINE         DT_IRQN_BY_NAME(SWINT_NODE, swint_pe0)
#define SWINT_PRIO             DT_IRQ_BY_NAME(SWINT_NODE, swint_pe0, priority)
/* Address of the software interrupt trigger register */
#define SWINT_REGISTER_ADDRESS DT_REG_ADDR(SWINT_NODE)

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

static void swint_handler(const void *unused)
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

	if (IS_ENABLED(CONFIG_ASSERT)) {
		/* SWINT cannot run with interrupts locked. */
		unsigned int key = arch_irq_lock();

		__ASSERT(arch_irq_unlocked(key), "irq_offload() called with interrupts locked");
		arch_irq_unlock(key);
	}

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
	IRQ_CONNECT(SWINT_IRQ_LINE, SWINT_PRIO, swint_handler, NULL, 0);
	irq_enable(SWINT_IRQ_LINE);
}

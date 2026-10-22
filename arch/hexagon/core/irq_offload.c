/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/irq.h>

/*
 * IRQ 0 is used as the software trigger for irq_offload.  The board DTS
 * assigns the lowest hardware IRQ at 0x0c (timer), so IRQ 0 does not
 * conflict with any real device on this platform.
 */
#define IRQ_OFFLOAD_LINE 0

static volatile irq_offload_routine_t offload_routine;
static const void *volatile offload_param;

static void irq_offload_isr(const void *param)
{
	ARG_UNUSED(param);

	irq_offload_routine_t tmp = offload_routine;
	const void *tmp_param = offload_param;

	offload_routine = NULL;
	if (tmp != NULL) {
		tmp(tmp_param);
	}
}

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	unsigned int key;

	key = irq_lock();
	offload_routine = routine;
	offload_param = parameter;

	/* Trigger software interrupt - fires when irq_unlock re-enables IE */
	hexagon_irq_trigger(IRQ_OFFLOAD_LINE);

	irq_unlock(key);
}

void arch_irq_offload_init(void)
{
	IRQ_CONNECT(IRQ_OFFLOAD_LINE, 0, irq_offload_isr, NULL, 0);
	irq_enable(IRQ_OFFLOAD_LINE);
}

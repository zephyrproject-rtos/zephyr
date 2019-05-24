/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/irq.c
 * @brief	NuttX libmetal irq definitions.
 */

#include <errno.h>
#include <metal/irq_controller.h>
#include <metal/alloc.h>
#include <nuttx/irq.h>

unsigned int metal_irq_save_disable(void)
{
	return up_irq_save();
}

void metal_irq_restore_enable(unsigned int flags)
{
	up_irq_restore(flags);
}

/* Implement the default irq controller */
static void metal_cntr_irq_set_enable(struct metal_irq_controller *cntr,
				      int irq, unsigned int enable)
{
	if (irq >= 0 && irq < cntr->irq_num) {
		if (enable == METAL_IRQ_ENABLE)
			up_enable_irq(irq);
		else
			up_disable_irq(irq);
	}
}

static int metal_cntr_irq_handler(int irq, void *context, void *data)
{
	if (context != NULL)
		return metal_irq_handle(data, irq);

	/* context == NULL mean unregister */
	irqchain_detach(irq, metal_cntr_irq_handler, data);
	sched_kfree(data);
	return 0;
}

static int metal_cntr_irq_attach(struct metal_irq_controller *cntr,
				 int irq, metal_irq_handler hd, void *arg)
{
	if (irq < 0 || irq >= cntr->irq_num)
		return -EINVAL;

	if (hd) {
		struct metal_irq *data;

		data = metal_allocate_memory(sizeof(*data));
		if (data == NULL)
			return -ENOMEM;

		data->hd  = hd;
		data->arg = arg;

		irq_attach(irq, metal_cntr_irq_handler, data);
	} else {
		unsigned int flags;

		flags = metal_irq_save_disable();
		irq_dispatch(irq, NULL); /* fake a irq request */
		metal_irq_restore_enable(flags);
	}

	return 0;
}

int metal_cntr_irq_init(void)
{
	static METAL_IRQ_CONTROLLER_DECLARE(metal_cntr_irq,
					    0, NR_IRQS,
					    NULL,
					    metal_cntr_irq_set_enable,
					    metal_cntr_irq_attach,
					    NULL)
	return metal_irq_register_controller(&metal_cntr_irq);
}

/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <metal/irq.h>
#include <metal/irq_controller.h>
#include <metal/list.h>
#include <metal/utilities.h>

/** List of registered IRQ controller */
static METAL_DECLARE_LIST(irq_cntrs);

static int metal_irq_allocate(int irq_base, int irq_num)
{
	struct metal_list *node;
	struct metal_irq_controller *cntr;
	int irq_tocheck = irq_base, irq_end_tocheck;

	if (irq_num == 0) {
		return METAL_IRQ_ANY;
	}
	if (irq_tocheck == METAL_IRQ_ANY) {
		irq_tocheck = 0;
	}
	irq_end_tocheck = irq_tocheck + irq_num;

	metal_list_for_each(&irq_cntrs, node) {
		int cntr_irq_base, cntr_irq_end;

		cntr = metal_container_of(node,
					  struct metal_irq_controller, node);
		cntr_irq_base = cntr->irq_base;
		cntr_irq_end = cntr_irq_base + cntr->irq_num;
		if (irq_tocheck < cntr_irq_end &&
		    irq_end_tocheck > cntr_irq_base) {
			if (irq_base != METAL_IRQ_ANY) {
				/* IRQ has been allocated */
				return METAL_IRQ_ANY;
			}
			irq_tocheck = cntr_irq_end;
			irq_end_tocheck = irq_tocheck + irq_num;
		}
	}
	return irq_tocheck;
}

int metal_irq_register_controller(struct metal_irq_controller *cntr)
{
	int irq_base;
	struct metal_list *node;

	if (cntr == NULL) {
		return -EINVAL;
	}
	metal_list_for_each(&irq_cntrs, node) {
		if (node == &cntr->node) {
			return 0;
		}
	}

	/* Allocate IRQ numbers which are not yet used by any IRQ
	 * controllers.*/
	irq_base = metal_irq_allocate(cntr->irq_base , cntr->irq_num);
	if (irq_base == METAL_IRQ_ANY) {
		return -EINVAL;
	}
	cntr->irq_base = irq_base;

	metal_list_add_tail(&irq_cntrs, &cntr->node);
	return 0;
}

static struct metal_irq_controller *metal_irq_get_controller(int irq)
{
	struct metal_list *node;
	struct metal_irq_controller *cntr;

	metal_list_for_each(&irq_cntrs, node) {
		int irq_base, irq_end;

		cntr = (struct metal_irq_controller *)
		       metal_container_of(node, struct metal_irq_controller,
				          node);
		irq_base = cntr->irq_base;
		irq_end = irq_base + cntr->irq_num;
		if (irq >= irq_base && irq < irq_end) {
		       return cntr;
		}
	}
	return NULL;
}

static void _metal_irq_set_enable(int irq, unsigned int state)
{
	struct metal_irq_controller *cntr;

	cntr = metal_irq_get_controller(irq);
	if (cntr == NULL) {
		return;
	}
	cntr->irq_set_enable(cntr, irq, state);
}

int metal_irq_register(int irq,
		       metal_irq_handler irq_handler,
		       void *arg)
{
	struct metal_irq_controller *cntr;
	struct metal_irq *irq_data;

	cntr = metal_irq_get_controller(irq);
	if (cntr == NULL) {
		return -EINVAL;
	}
	if (cntr->irq_register != NULL) {
		return cntr->irq_register(cntr, irq, irq_handler, arg);
	}
	if (cntr->irqs == NULL) {
		return -EINVAL;
	}
	irq_data = &cntr->irqs[irq - cntr->irq_base];
	irq_data->hd = irq_handler;
	irq_data->arg = arg;
	return 0;
}

void metal_irq_enable(unsigned int vector)
{
	_metal_irq_set_enable((int)vector, METAL_IRQ_ENABLE);
}

void metal_irq_disable(unsigned int vector)
{
	_metal_irq_set_enable((int)vector, METAL_IRQ_DISABLE);
}

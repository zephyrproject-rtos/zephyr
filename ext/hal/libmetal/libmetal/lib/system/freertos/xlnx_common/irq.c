/*
 * Copyright (c) 2016 - 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/xlnx_common/irq.c
 * @brief	generic libmetal Xilinx irq controller definitions.
 */

#include <errno.h>
#include <metal/irq_controller.h>
#include <metal/sys.h>
#include <metal/log.h>
#include <metal/mutex.h>
#include <metal/list.h>
#include <metal/utilities.h>
#include <metal/alloc.h>

#define MAX_IRQS XLNX_MAXIRQS

static struct metal_irq irqs[MAX_IRQS]; /**< Linux IRQs array */

static void metal_xlnx_irq_set_enable(struct metal_irq_controller *irq_cntr,
				     int irq, unsigned int state)
{
	if (irq < irq_cntr->irq_base ||
	    irq >= irq_cntr->irq_base + irq_cntr->irq_num) {
		metal_log(METAL_LOG_ERROR, "%s: invalid irq %d\n",
			  __func__, irq);
		return;
	} else if (state == METAL_IRQ_ENABLE) {
		sys_irq_enable((unsigned int)irq);
	} else {
		sys_irq_disable((unsigned int)irq);
	}
}

/**< Xilinx common platform IRQ controller */
static METAL_IRQ_CONTROLLER_DECLARE(xlnx_irq_cntr,
				    0, MAX_IRQS,
				    NULL,
				    metal_xlnx_irq_set_enable, NULL,
				    irqs)

/**
 * @brief default handler
 */
void metal_xlnx_irq_isr(void *arg)
{
	unsigned int vector;

	vector = (uintptr_t)arg;
	if (vector >= MAX_IRQS) {
		return;
	}
	(void)metal_irq_handle(&irqs[vector], (int)vector);
}

int metal_xlnx_irq_init(void)
{
	int ret;

	ret =  metal_irq_register_controller(&xlnx_irq_cntr);
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "%s: register irq controller failed.\n",
			  __func__);
		return ret;
	}
	return 0;
}

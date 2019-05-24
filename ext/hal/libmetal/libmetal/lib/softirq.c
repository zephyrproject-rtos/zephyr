/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <metal/atomic.h>
#include <metal/irq.h>
#include <metal/irq_controller.h>
#include <metal/log.h>
#include <metal/sys.h>
#include <metal/softirq.h>
#include <metal/utilities.h>
#include <string.h>

#define METAL_SOFTIRQ_NUM 64

#define METAL_SOFTIRQ_ARRAY_DECLARE(num) \
	static const int metal_softirq_num = num; \
	static struct metal_irq metal_softirqs[num]; \
	static atomic_char metal_softirq_pending[num]; \
	static atomic_char metal_softirq_enabled[num]; \

static int metal_softirq_avail = 0;
METAL_SOFTIRQ_ARRAY_DECLARE(METAL_SOFTIRQ_NUM)

static void metal_softirq_set_enable(struct metal_irq_controller *cntr,
				     int irq, unsigned int enable)
{
	if (irq < cntr->irq_base ||
	    irq >= (cntr->irq_base + cntr->irq_num)) {
		return;
	}

	irq -= cntr->irq_base;
	if (enable ==  METAL_IRQ_ENABLE) {
		atomic_store(&metal_softirq_enabled[irq], 1);
	} else {
		atomic_store(&metal_softirq_enabled[irq], 0);
	}
}

static METAL_IRQ_CONTROLLER_DECLARE(metal_softirq_cntr,
				    METAL_IRQ_ANY, METAL_SOFTIRQ_NUM,
				    NULL,
				    metal_softirq_set_enable, NULL,
				    metal_softirqs)

void metal_softirq_set(int irq)
{
	struct metal_irq_controller *cntr;

	cntr = &metal_softirq_cntr;

	if (irq < cntr->irq_base ||
	    irq >= (cntr->irq_base + cntr->irq_num)) {
		return;
	}

	irq -= cntr->irq_base;
	atomic_store(&metal_softirq_pending[irq], 1);
}

int metal_softirq_init()
{
	return metal_irq_register_controller(&metal_softirq_cntr);
}

int metal_softirq_allocate(int num)
{
	int irq_base;

	if ((metal_softirq_avail + num) >= metal_softirq_num) {
		metal_log(METAL_LOG_ERROR, "No %d available soft irqs.\r\n",
			  num);
		return -EINVAL;
	}
	irq_base = metal_softirq_avail;
	irq_base += metal_softirq_cntr.irq_base;
	metal_softirq_avail += num;
	return irq_base;
}

void metal_softirq_dispatch()
{
	int i;

	for (i = 0; i < metal_softirq_num; i++) {
		struct metal_irq *irq;
		char is_pending = 1;

		if (atomic_load(&metal_softirq_enabled[i]) != 0 &&
		    atomic_compare_exchange_strong(&metal_softirq_pending[i],
						   &is_pending, 0)) {
			irq = &metal_softirqs[i];
			(void)metal_irq_handle(irq,
					       i + metal_softirq_cntr.irq_base);
		}
	}
}

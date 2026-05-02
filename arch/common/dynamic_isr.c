/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sw_isr_common.h"
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>

void __weak z_isr_install(unsigned int irq, void (*routine)(const void *),
			  const void *param)
{
	unsigned int table_idx;

	/*
	 * Do not assert on the IRQ enable status for ARM GIC since the SGI
	 * type interrupts are always enabled and attempting to install an ISR
	 * for them will cause the assertion to fail.
	 */
#ifndef CONFIG_GIC
	__ASSERT(!irq_is_enabled(irq), "IRQ %d is enabled", irq);
#endif /* !CONFIG_GIC */

	table_idx = z_get_sw_isr_table_idx(irq);

	/* If dynamic IRQs are enabled, then the _sw_isr_table is in RAM and
	 * can be modified
	 */
	_sw_isr_table[table_idx].arg = param;
	_sw_isr_table[table_idx].isr = routine;
}

/* Some architectures don't/can't interpret flags or priority and have
 * no more processing to do than this.  Provide a generic fallback.
 */
int __weak arch_irq_connect_dynamic(unsigned int irq,
				    unsigned int priority,
				    void (*routine)(const void *),
				    const void *parameter,
				    uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}

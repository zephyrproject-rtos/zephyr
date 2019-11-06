/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sw_isr_table.h>

#ifdef CONFIG_DYNAMIC_INTERRUPTS
unsigned int _irq_line_get(void)
{
	unsigned int irqn;

	__asm__ volatile("mrs %0, IPSR\n\t" : "=r"(irqn));

	/* Adjust from ARM exception to interrupt number. */
	return irqn - 16;
}

void nrf_sw_isr_direct(void)
{
	unsigned int irqn = _irq_line_get();

	if (irqn < IRQ_TABLE_SIZE) {
		struct _isr_table_entry *irq = &_sw_isr_table[irqn];

		irq->isr(irq->arg);
	}
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

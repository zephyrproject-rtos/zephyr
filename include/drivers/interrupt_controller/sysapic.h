/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SYSAPIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_SYSAPIC_H_

#include <drivers/interrupt_controller/loapic.h>

#define IRQ_TRIGGER_EDGE	IOAPIC_EDGE
#define IRQ_TRIGGER_LEVEL	IOAPIC_LEVEL

#define IRQ_POLARITY_HIGH	IOAPIC_HIGH
#define IRQ_POLARITY_LOW	IOAPIC_LOW

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#define LOAPIC_IRQ_BASE  CONFIG_IOAPIC_NUM_RTES
#define LOAPIC_IRQ_COUNT 6  /* Default to LOAPIC_TIMER to LOAPIC_ERROR */

void z_irq_controller_irq_config(unsigned int vector, unsigned int irq,
				 uint32_t flags);

int z_irq_controller_isr_vector_get(void);

static inline void z_irq_controller_eoi(void)
{
	x86_write_loapic(LOAPIC_EOI, 0);
}

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SYSAPIC_H_ */

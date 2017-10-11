/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_SYS_APIC_H
#define __INC_SYS_APIC_H

#include <drivers/loapic.h>

#define _IRQ_TRIGGER_EDGE	IOAPIC_EDGE
#define _IRQ_TRIGGER_LEVEL	IOAPIC_LEVEL

#define _IRQ_POLARITY_HIGH	IOAPIC_HIGH
#define _IRQ_POLARITY_LOW	IOAPIC_LOW

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#define LOAPIC_IRQ_BASE  CONFIG_IOAPIC_NUM_RTES
#define LOAPIC_IRQ_COUNT 6  /* Default to LOAPIC_TIMER to LOAPIC_ERROR */

/* irq_controller.h interface */
void __irq_controller_irq_config(unsigned int vector, unsigned int irq,
				 u32_t flags);

int __irq_controller_isr_vector_get(void);

#ifdef CONFIG_JAILHOUSE_X2APIC
void _jailhouse_eoi(void);
#endif

static inline void __irq_controller_eoi(void)
{
#if CONFIG_EOI_FORWARDING_BUG
	_lakemont_eoi();
#else
	*(volatile int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_EOI) = 0;
#endif
}

#else /* _ASMLANGUAGE */

#if CONFIG_EOI_FORWARDING_BUG
.macro __irq_controller_eoi_macro
	call	_lakemont_eoi
.endm
#else
.macro __irq_controller_eoi_macro
#ifdef CONFIG_JAILHOUSE_X2APIC
	call	_jailhouse_eoi
#else
	xorl %eax, %eax			/* zeroes eax */
	loapic_eoi_reg = (CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_EOI)
	movl %eax, loapic_eoi_reg	/* tell LOAPIC the IRQ is handled */
#endif
.endm
#endif /* CONFIG_EOI_FORWARDING_BUG */

#endif /* _ASMLANGUAGE */

#endif /* __INC_SYS_APIC_H */

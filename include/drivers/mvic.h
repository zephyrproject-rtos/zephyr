/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MVIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_MVIC_H_

#include <arch/cpu.h>

/* Register defines. A lot of similarities to APIC, but not quite the same */
#define MVIC_TPR	0xFEE00080	/* Task priority register */
#define MVIC_PPR	0xFEE000A0	/* Process priority register */
#define MVIC_EOI	0xFEE000B0	/* End-of-interrupt register */
#define MVIC_SIVR	0xFEE000F0	/* Spurious interrupt vector register */
#define MVIC_ISR	0xFEE00110	/* In-service register */
#define MVIC_IRR	0xFEE00210	/* Interrupt request register */
#define MVIC_LVTTIMER	0xFEE00320	/* Local vector table timer register */
#define MVIC_ICR	0xFEE00380	/* Timer initial count register */
#define MVIC_CCR	0xFEE00390	/* Timer current count register */
#define MVIC_IOREGSEL	0xFEC00000	/* Register select (index) */
#define MVIC_IOWIN	0xFEC00010	/* Register windows (data) */

/* MVIC_LVTTIMER bits */
#define MVIC_LVTTIMER_MASK	BIT(16)
#define MVIC_LVTTIMER_PERIODIC	BIT(17)

/* MVIC_IOWIN bits */
#define MVIC_IOWIN_TRIGGER_LEVEL	BIT(15)
#define MVIC_IOWIN_TRIGGER_EDGE		0
#define MVIC_IOWIN_MASK			BIT(16)
#define MVIC_IOWIN_SUPPORTED_BITS_MASK	(MVIC_IOWIN_MASK | \
					 MVIC_IOWIN_TRIGGER_LEVEL)

/* MVIC IOREGSEL register usage defines */
#define MVIC_LOW_NIBBLE_MASK	0x07
#define MVIC_HIGH_NIBBLE_MASK	0x18

#define MVIC_NUM_RTES		32

#define _IRQ_TRIGGER_EDGE	MVIC_IOWIN_TRIGGER_EDGE
#define _IRQ_TRIGGER_LEVEL	MVIC_IOWIN_TRIGGER_LEVEL

/* MVIC does not support IRQ_POLARITY_HIGH or IRQ_POLARITY_LOW,
 * leave undefined
 */

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/* Implementation of irq_controller.h interface */

#define __IRQ_CONTROLLER_VECTOR_MAPPING(irq)	((irq) + 32)

void __irq_controller_irq_config(unsigned int vector, unsigned int irq,
				 u32_t flags);

int __irq_controller_isr_vector_get(void);

static inline void __irq_controller_eoi(void)
{
	*(volatile int *)(MVIC_EOI) = 0;
}

#else /* _ASMLANGUAGE */

.macro __irq_controller_eoi_macro
	xorl %eax, %eax			/* zeroes eax */
	movl %eax, MVIC_EOI		/* tell MVIC the IRQ is handled */
.endm

#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MVIC_H_ */

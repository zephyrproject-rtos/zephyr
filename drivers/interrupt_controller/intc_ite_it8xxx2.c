/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <sys/printk.h>
#include <sw_isr_table.h>
#include "intc_ite_it8xxx2.h"

#define MAX_REGISR_IRQ_NUM		8
#define IVECT_OFFSET_WITH_IRQ		0x10
#define SOFT_INTC_IRQ			161	/* software interrupt */

static volatile uint8_t *const reg_status[MAX_ISR_REG_NUM] = {
	&ISR0, &ISR1, &ISR2, &ISR3,
	&ISR4, &ISR5, &ISR6, &ISR7,
	&ISR8, &ISR9, &ISR10, &ISR11,
	&ISR12, &ISR13, &ISR14, &ISR15,
	&ISR16, &ISR17, &ISR18, &ISR19,
	&ISR20
};

static volatile uint8_t *const reg_enable[MAX_ISR_REG_NUM] = {
	&IER0, &IER1, &IER2, &IER3,
	&IER4, &IER5, &IER6, &IER7,
	&IER8, &IER9, &IER10, &IER11,
	&IER12, &IER13, &IER14, &IER15,
	&IER16, &IER17, &IER18, &IER19,
	&IER20
};

/* edge/level trigger register */
static volatile uint8_t *const reg_ielmr[] = {
	&IELMR0, &IELMR1, &IELMR2, &IELMR3,
	&IELMR4, &IELMR5, &IELMR6, &IELMR7,
	&IELMR8, &IELMR9, &IELMR10, &IELMR11,
	&IELMR12, &IELMR13, &IELMR14, &IELMR15,
	&IELMR16, &IELMR17, &IELMR18, &IELMR19,
	&IELMR20
};

/* high/low trigger register */
static volatile uint8_t *const reg_ipolr[] = {
	&IPOLR0, &IPOLR1, &IPOLR2, &IPOLR3,
	&IPOLR4, &IPOLR5, &IPOLR6, &IPOLR7,
	&IPOLR8, &IPOLR9, &IPOLR10, &IPOLR11,
	&IPOLR12, &IPOLR13, &IPOLR14, &IPOLR15,
	&IPOLR16, &IPOLR17, &IPOLR18, &IPOLR19,
	&IPOLR20
};

inline void set_csr(unsigned long bit)
{
	unsigned long __tmp;

	if (__builtin_constant_p(bit) && (bit) < 32) {
		__asm__ volatile \
		("csrrs %0, mie, %1" : "=r" (__tmp) : "i" (bit));
	} else {
		__asm__ volatile \
		("csrrs %0, mie, %1" : "=r" (__tmp) : "r" (bit));
	}
}

static void ite_intc_isr_clear(unsigned int irq)
{
	uint32_t g, i;
	volatile uint8_t *isr;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;
	isr = reg_status[g];
	*isr = BIT(i);
}

void ite_intc_irq_enable(unsigned int irq)
{
	uint32_t g, i;
	volatile uint8_t *en;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;
	en = reg_enable[g];
	SET_MASK(*en, BIT(i));
}

void ite_intc_irq_disable(unsigned int irq)
{
	uint32_t g, i;
	volatile uint8_t *en;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;
	en = reg_enable[g];
	CLEAR_MASK(*en, BIT(i));
}

void ite_intc_irq_priority_set(unsigned int irq,
		unsigned int prio, unsigned int flags)
{
	uint32_t g, i;
	volatile uint8_t *tri;

	if ((irq > CONFIG_NUM_IRQS) || (flags&IRQ_TYPE_EDGE_BOTH)) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;
	tri = reg_ipolr[g];
	if ((flags&IRQ_TYPE_LEVEL_HIGH) || (flags&IRQ_TYPE_EDGE_RISING)) {
		CLEAR_MASK(*tri, BIT(i));
	} else {
		SET_MASK(*tri, BIT(i));
	}
	tri = reg_ielmr[g];
	if ((flags&IRQ_TYPE_LEVEL_LOW) || (flags&IRQ_TYPE_LEVEL_HIGH)) {
		CLEAR_MASK(*tri, BIT(i));
	} else {
		SET_MASK(*tri, BIT(i));
	}
}

int ite_intc_irq_is_enable(unsigned int irq)
{
	uint32_t g, i;
	volatile uint8_t *en;

	if (irq > CONFIG_NUM_IRQS) {
		return 0;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;
	en = reg_enable[g];
	return IS_MASK_SET(*en, BIT(i));
}

void ite_intc_irq_handler(const void *arg)
{
	ARG_UNUSED(arg);
	uint8_t irq = IVECT1 - IVECT_OFFSET_WITH_IRQ;
	struct _isr_table_entry *ite;

	/* software interrupt isr*/
	if ((irq < CONFIG_NUM_IRQS) && (irq > 0)) {
		ite = (struct _isr_table_entry *)&_sw_isr_table[irq];
		ite_intc_isr_clear(irq);
		ite->isr(ite->arg);
	} else {
		z_irq_spurious(NULL);
	}
}

uint8_t get_irq(void *arg)
{
	ARG_UNUSED(arg);
	uint8_t irq = IVECT1 - IVECT_OFFSET_WITH_IRQ;

	ite_intc_isr_clear(irq);
	return irq;
}

static int ite_intc_init(const struct device *dev)
{
	irq_connect_dynamic(SOFT_INTC_IRQ, 0, &ite_intc_irq_handler, NULL, 0);
	ite_intc_irq_enable(SOFT_INTC_IRQ);
	irq_unlock(0);

	/* GIE enable */
	set_csr(MIP_MEIP);
	return 0;
}

SYS_INIT(ite_intc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

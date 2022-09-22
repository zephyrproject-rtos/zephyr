/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_it8xxx2, LOG_LEVEL_DBG);
#include <zephyr/sys/printk.h>
#include <zephyr/sw_isr_table.h>
#include "intc_ite_it8xxx2.h"

#define MAX_REGISR_IRQ_NUM		8
#define IVECT_OFFSET_WITH_IRQ		0x10

/* Interrupt number of INTC module */
static uint8_t intc_irq;

static volatile uint8_t *const reg_status[] = {
	&ISR0, &ISR1, &ISR2, &ISR3,
	&ISR4, &ISR5, &ISR6, &ISR7,
	&ISR8, &ISR9, &ISR10, &ISR11,
	&ISR12, &ISR13, &ISR14, &ISR15,
	&ISR16, &ISR17, &ISR18, &ISR19,
	&ISR20, &ISR21, &ISR22, &ISR23
};

static volatile uint8_t *const reg_enable[] = {
	&IER0, &IER1, &IER2, &IER3,
	&IER4, &IER5, &IER6, &IER7,
	&IER8, &IER9, &IER10, &IER11,
	&IER12, &IER13, &IER14, &IER15,
	&IER16, &IER17, &IER18, &IER19,
	&IER20, &IER21, &IER22, &IER23
};

/* edge/level trigger register */
static volatile uint8_t *const reg_ielmr[] = {
	&IELMR0, &IELMR1, &IELMR2, &IELMR3,
	&IELMR4, &IELMR5, &IELMR6, &IELMR7,
	&IELMR8, &IELMR9, &IELMR10, &IELMR11,
	&IELMR12, &IELMR13, &IELMR14, &IELMR15,
	&IELMR16, &IELMR17, &IELMR18, &IELMR19,
	&IELMR20, &IELMR21, &IELMR22, &IELMR23,
};

/* high/low trigger register */
static volatile uint8_t *const reg_ipolr[] = {
	&IPOLR0, &IPOLR1, &IPOLR2, &IPOLR3,
	&IPOLR4, &IPOLR5, &IPOLR6, &IPOLR7,
	&IPOLR8, &IPOLR9, &IPOLR10, &IPOLR11,
	&IPOLR12, &IPOLR13, &IPOLR14, &IPOLR15,
	&IPOLR16, &IPOLR17, &IPOLR18, &IPOLR19,
	&IPOLR20, &IPOLR21, &IPOLR22, &IPOLR23
};

#define IT8XXX2_IER_COUNT ARRAY_SIZE(reg_enable)
static uint8_t ier_setting[IT8XXX2_IER_COUNT];

void ite_intc_save_and_disable_interrupts(void)
{
	volatile uint8_t _ier __unused;
	/* Disable global interrupt for critical section */
	unsigned int key = irq_lock();

	/* Save and disable interrupts */
	for (int i = 0; i < IT8XXX2_IER_COUNT; i++) {
		ier_setting[i] = *reg_enable[i];
		*reg_enable[i] = 0;
	}
	/*
	 * This load operation will guarantee the above modification of
	 * SOC's register can be seen by any following instructions.
	 * Note: Barrier instruction can not synchronize chip register,
	 * so we introduce workaround here.
	 */
	_ier = *reg_enable[IT8XXX2_IER_COUNT - 1];
	irq_unlock(key);
}

void ite_intc_restore_interrupts(void)
{
	/*
	 * Ensure the highest priority interrupt will be the first fired
	 * interrupt when soc is ready to go.
	 */
	unsigned int key = irq_lock();

	/* Restore interrupt state */
	for (int i = 0; i < IT8XXX2_IER_COUNT; i++) {
		*reg_enable[i] = ier_setting[i];
	}
	irq_unlock(key);
}

void ite_intc_isr_clear(unsigned int irq)
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

void __soc_ram_code ite_intc_irq_enable(unsigned int irq)
{
	uint32_t g, i;
	volatile uint8_t *en;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;
	en = reg_enable[g];

	/* critical section due to run a bit-wise OR operation */
	unsigned int key = irq_lock();
	SET_MASK(*en, BIT(i));
	irq_unlock(key);
}

void __soc_ram_code ite_intc_irq_disable(unsigned int irq)
{
	uint32_t g, i;
	volatile uint8_t *en;
	volatile uint8_t _ier __unused;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;
	en = reg_enable[g];

	/* critical section due to run a bit-wise OR operation */
	unsigned int key = irq_lock();
	CLEAR_MASK(*en, BIT(i));
	/*
	 * This load operation will guarantee the above modification of
	 * SOC's register can be seen by any following instructions.
	 */
	_ier = *en;
	irq_unlock(key);
}

void ite_intc_irq_polarity_set(unsigned int irq, unsigned int flags)
{
	uint32_t g, i;
	volatile uint8_t *tri;

	if ((irq > CONFIG_NUM_IRQS) || ((flags&IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH)) {
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

int __soc_ram_code ite_intc_irq_is_enable(unsigned int irq)
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

uint8_t __soc_ram_code ite_intc_get_irq_num(void)
{
	return intc_irq;
}

bool __soc_ram_code ite_intc_no_irq(void)
{
	return (IVECT == IVECT_OFFSET_WITH_IRQ);
}

uint8_t __soc_ram_code get_irq(void *arg)
{
	ARG_UNUSED(arg);

	/* wait until two equal interrupt values are read */
	do {
		/* Read interrupt number from interrupt vector register */
		intc_irq = IVECT;
		/*
		 * WORKAROUND: when the interrupt vector register (IVECT)
		 * isn't latched in a load operation, we read it again to make
		 * sure the value we got is the correct value.
		 */
	} while (intc_irq != IVECT);
	/* determine interrupt number */
	intc_irq -= IVECT_OFFSET_WITH_IRQ;
	/*
	 * Look for pending interrupt if there's interrupt number 0 from
	 * the AIVECT register.
	 */
	if (intc_irq == 0) {
		uint8_t int_pending;

		for (int i = (IT8XXX2_IER_COUNT - 1); i >= 0; i--) {
			int_pending = (*reg_status[i] & *reg_enable[i]);
			if (int_pending != 0) {
				intc_irq = (MAX_REGISR_IRQ_NUM * i) +
						find_msb_set(int_pending) - 1;
				LOG_DBG("Pending interrupt found: %d",
						intc_irq);
				LOG_DBG("CPU mepc: 0x%lx", csr_read(mepc));
				break;
			}
		}
	}
	/* clear interrupt status */
	ite_intc_isr_clear(intc_irq);
	/* return interrupt number */
	return intc_irq;
}

void ite_intc_init(void)
{
	/* Ensure interrupts of soc are disabled at default */
	for (int i = 0; i < ARRAY_SIZE(reg_enable); i++)
		*reg_enable[i] = 0;

	/* Enable M-mode external interrupt */
	csr_set(mie, MIP_MEIP);
}

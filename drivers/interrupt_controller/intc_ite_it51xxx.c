/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-it51xxx-intc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(intc_ite_it51xxx, LOG_LEVEL_DBG);

/* it51xxx INTC registers definition */
#define INTC_REG_OFFSET(n) ((n) < 3 ? 1 : 2)
#define INTC_GRPNISR(n)    (4 * ((n) + INTC_REG_OFFSET(n)))
#define INTC_GRPNIER(n)    (4 * ((n) + INTC_REG_OFFSET(n)) + 1)
#define INTC_GRPNIELMR(n)  (4 * ((n) + INTC_REG_OFFSET(n)) + 2)
#define INTC_GRPNIPOLR(n)  (4 * ((n) + INTC_REG_OFFSET(n)) + 3)
#define INTC_IVECT         0x10

#define II51XXX_INTC_GROUP_COUNT 29
#define MAX_REGISR_IRQ_NUM       8
#define IVECT_OFFSET_WITH_IRQ    0x10

static mm_reg_t intc_base = DT_REG_ADDR(DT_NODELABEL(intc));
/* Interrupt number of INTC module */
static uint8_t intc_irq;
static uint8_t ier_setting[II51XXX_INTC_GROUP_COUNT];

void ite_intc_save_and_disable_interrupts(void)
{
	volatile uint8_t _ier __unused;
	/* Disable global interrupt for critical section */
	unsigned int key = irq_lock();

	/* Save and disable interrupts */
	for (int i = 0; i < II51XXX_INTC_GROUP_COUNT; i++) {
		ier_setting[i] = sys_read8(intc_base + INTC_GRPNIER(i));
		sys_write8(0, intc_base + INTC_GRPNIER(i));
	}
	/*
	 * This load operation will guarantee the above modification of
	 * SOC's register can be seen by any following instructions.
	 * Note: Barrier instruction can not synchronize chip register,
	 * so we introduce workaround here.
	 */
	_ier = sys_read8(intc_base + INTC_GRPNIER(II51XXX_INTC_GROUP_COUNT - 1));

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
	for (int i = 0; i < II51XXX_INTC_GROUP_COUNT; i++) {
		sys_write8(ier_setting[i], intc_base + INTC_GRPNIER(i));
	}

	irq_unlock(key);
}

void ite_intc_isr_clear(unsigned int irq)
{
	uint32_t g, i;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;

	sys_write8(BIT(i), intc_base + INTC_GRPNISR(g));
}

void ite_intc_irq_enable(unsigned int irq)
{
	uint32_t g, i;
	uint8_t en;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;

	/* critical section due to run a bit-wise OR operation */
	unsigned int key = irq_lock();

	en = sys_read8(intc_base + INTC_GRPNIER(g));
	sys_write8(en | BIT(i), intc_base + INTC_GRPNIER(g));

	irq_unlock(key);
}

void ite_intc_irq_disable(unsigned int irq)
{
	uint32_t g, i;
	uint8_t en;

	volatile uint8_t _ier __unused;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;

	/* critical section due to run a bit-wise OR operation */
	unsigned int key = irq_lock();

	en = sys_read8(intc_base + INTC_GRPNIER(g));
	sys_write8(en & ~BIT(i), intc_base + INTC_GRPNIER(g));
	/*
	 * This load operation will guarantee the above modification of
	 * SOC's register can be seen by any following instructions.
	 */
	_ier = sys_read8(intc_base + INTC_GRPNIER(g));

	irq_unlock(key);
}

void ite_intc_irq_polarity_set(unsigned int irq, unsigned int flags)
{
	uint32_t g, i;
	uint8_t tri;

	if ((irq > CONFIG_NUM_IRQS) || ((flags & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH)) {
		return;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;

	tri = sys_read8(intc_base + INTC_GRPNIPOLR(g));
	if ((flags & IRQ_TYPE_LEVEL_HIGH) || (flags & IRQ_TYPE_EDGE_RISING)) {
		sys_write8(tri & ~BIT(i), intc_base + INTC_GRPNIPOLR(g));
	} else {
		sys_write8(tri | BIT(i), intc_base + INTC_GRPNIPOLR(g));
	}

	tri = sys_read8(intc_base + INTC_GRPNIELMR(g));
	if ((flags & IRQ_TYPE_LEVEL_LOW) || (flags & IRQ_TYPE_LEVEL_HIGH)) {
		sys_write8(tri & ~BIT(i), intc_base + INTC_GRPNIELMR(g));
	} else {
		sys_write8(tri | BIT(i), intc_base + INTC_GRPNIELMR(g));
	}

	/* W/C interrupt status of the pin */
	sys_write8(BIT(i), intc_base + INTC_GRPNISR(g));
}

int ite_intc_irq_is_enable(unsigned int irq)
{
	uint32_t g, i;
	uint8_t en;

	if (irq > CONFIG_NUM_IRQS) {
		return 0;
	}
	g = irq / MAX_REGISR_IRQ_NUM;
	i = irq % MAX_REGISR_IRQ_NUM;

	en = sys_read8(intc_base + INTC_GRPNIER(g));

	return (en & BIT(i));
}

uint8_t ite_intc_get_irq_num(void)
{
	return intc_irq;
}

uint8_t get_irq(void *arg)
{
	ARG_UNUSED(arg);
	/* wait until two equal interrupt values are read */
	do {
		/* Read interrupt number from interrupt vector register */
		intc_irq = sys_read8(intc_base + INTC_IVECT);
		/*
		 * WORKAROUND: when the interrupt vector register (INTC_VECT)
		 * isn't latched in a load operation, we read it again to make
		 * sure the value we got is the correct value.
		 */
	} while (intc_irq != sys_read8(intc_base + INTC_IVECT));
	/* determine interrupt number */
	intc_irq -= IVECT_OFFSET_WITH_IRQ;
	/* clear interrupt status */
	ite_intc_isr_clear(intc_irq);
	/* return interrupt number */
	return intc_irq;
}

void soc_interrupt_init(void)
{
	/* Ensure interrupts of soc are disabled at default */
	for (int i = 0; i < II51XXX_INTC_GROUP_COUNT; i++) {
		sys_write8(0, intc_base + INTC_GRPNIER(i));
	}

	/* Enable M-mode external interrupt */
	csr_set(mie, MIP_MEIP);
}

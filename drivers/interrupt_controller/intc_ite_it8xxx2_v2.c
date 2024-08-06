/*
 * Copyright (c) 2023 ITE Corporation. All Rights Reserved
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sw_isr_table.h>
#include "intc_ite_it8xxx2.h"
LOG_MODULE_REGISTER(intc_it8xxx2_v2, LOG_LEVEL_DBG);

#define IT8XXX2_INTC_BASE		DT_REG_ADDR(DT_NODELABEL(intc))
#define IT8XXX2_INTC_BASE_SHIFT(g)	(IT8XXX2_INTC_BASE + ((g) << 2))

/* Interrupt status register */
#define IT8XXX2_INTC_ISR(g)	ECREG(IT8XXX2_INTC_BASE_SHIFT(g) + \
				((g) < 4 ? 0x0 : 0x4))
/* Interrupt enable register */
#define IT8XXX2_INTC_IER(g)	ECREG(IT8XXX2_INTC_BASE_SHIFT(g) + \
				((g) < 4 ? 0x1 : 0x5))
/* Interrupt edge/level triggered mode register */
#define IT8XXX2_INTC_IELMR(g)	ECREG(IT8XXX2_INTC_BASE_SHIFT(g) + \
				((g) < 4 ? 0x2 : 0x6))
/* Interrupt polarity register */
#define IT8XXX2_INTC_IPOLR(g)	ECREG(IT8XXX2_INTC_BASE_SHIFT(g) + \
				((g) < 4 ? 0x3 : 0x7))

#define IT8XXX2_INTC_GROUP_CNT      24
#define MAX_REGISR_IRQ_NUM          8
#define IVECT_OFFSET_WITH_IRQ       0x10

/* Interrupt number of INTC module */
static uint8_t intc_irq;
static uint8_t ier_setting[IT8XXX2_INTC_GROUP_CNT];

void ite_intc_save_and_disable_interrupts(void)
{
	/* Disable global interrupt for critical section */
	unsigned int key = irq_lock();

	/* Save and disable interrupts */
	for (int i = 0; i < IT8XXX2_INTC_GROUP_CNT; i++) {
		ier_setting[i] = IT8XXX2_INTC_IER(i);
		IT8XXX2_INTC_IER(i) = 0;
	}

	/*
	 * This load operation will guarantee the above modification of
	 * SOC's register can be seen by any following instructions.
	 * Note: Barrier instruction can not synchronize chip register,
	 * so we introduce workaround here.
	 */
	IT8XXX2_INTC_IER(IT8XXX2_INTC_GROUP_CNT - 1);

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
	for (int i = 0; i < IT8XXX2_INTC_GROUP_CNT; i++) {
		IT8XXX2_INTC_IER(i) = ier_setting[i];
	}

	irq_unlock(key);
}

void ite_intc_isr_clear(unsigned int irq)
{
	uint32_t group, index;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}

	group = irq / MAX_REGISR_IRQ_NUM;
	index = irq % MAX_REGISR_IRQ_NUM;

	IT8XXX2_INTC_ISR(group) = BIT(index);
}

void __soc_ram_code ite_intc_irq_enable(unsigned int irq)
{
	uint32_t group, index;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}

	group = irq / MAX_REGISR_IRQ_NUM;
	index = irq % MAX_REGISR_IRQ_NUM;

	/* Critical section due to run a bit-wise OR operation */
	unsigned int key = irq_lock();

	IT8XXX2_INTC_IER(group) |= BIT(index);

	irq_unlock(key);
}

void __soc_ram_code ite_intc_irq_disable(unsigned int irq)
{
	uint32_t group, index;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}

	group = irq / MAX_REGISR_IRQ_NUM;
	index = irq % MAX_REGISR_IRQ_NUM;

	/* Critical section due to run a bit-wise NAND operation */
	unsigned int key = irq_lock();

	IT8XXX2_INTC_IER(group) &= ~BIT(index);
	/*
	 * This load operation will guarantee the above modification of
	 * SOC's register can be seen by any following instructions.
	 */
	IT8XXX2_INTC_IER(group);

	irq_unlock(key);
}

void ite_intc_irq_polarity_set(unsigned int irq, unsigned int flags)
{
	uint32_t group, index;

	if (irq > CONFIG_NUM_IRQS) {
		return;
	}

	if ((flags & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH) {
		return;
	}

	group = irq / MAX_REGISR_IRQ_NUM;
	index = irq % MAX_REGISR_IRQ_NUM;

	if ((flags & IRQ_TYPE_LEVEL_HIGH) || (flags & IRQ_TYPE_EDGE_RISING)) {
		IT8XXX2_INTC_IPOLR(group) &= ~BIT(index);
	} else {
		IT8XXX2_INTC_IPOLR(group) |= BIT(index);
	}

	if ((flags & IRQ_TYPE_LEVEL_LOW) || (flags & IRQ_TYPE_LEVEL_HIGH)) {
		IT8XXX2_INTC_IELMR(group) &= ~BIT(index);
	} else {
		IT8XXX2_INTC_IELMR(group) |= BIT(index);
	}
}

int __soc_ram_code ite_intc_irq_is_enable(unsigned int irq)
{
	uint32_t group, index;

	if (irq > CONFIG_NUM_IRQS) {
		return 0;
	}

	group = irq / MAX_REGISR_IRQ_NUM;
	index = irq % MAX_REGISR_IRQ_NUM;

	return IS_MASK_SET(IT8XXX2_INTC_IER(group), BIT(index));
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

	/* Wait until two equal interrupt values are read */
	do {
		/* Read interrupt number from interrupt vector register */
		intc_irq = IVECT;
		/*
		 * WORKAROUND: when the interrupt vector register (IVECT)
		 * isn't latched in a load operation, we read it again to make
		 * sure the value we got is the correct value.
		 */
	} while (intc_irq != IVECT);

	/* Determine interrupt number */
	intc_irq -= IVECT_OFFSET_WITH_IRQ;

	/*
	 * Look for pending interrupt if there's interrupt number 0 from
	 * the AIVECT register.
	 */
	if (intc_irq == 0) {
		uint8_t int_pending;

		for (int i = (IT8XXX2_INTC_GROUP_CNT - 1); i >= 0; i--) {
			int_pending =
				(IT8XXX2_INTC_ISR(i) & IT8XXX2_INTC_IER(i));
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

	/* Clear interrupt status */
	ite_intc_isr_clear(intc_irq);

	/* Return interrupt number */
	return intc_irq;
}

static void intc_irq0_handler(const void *arg)
{
	ARG_UNUSED(arg);

	LOG_DBG("SOC it8xxx2 Interrupt 0 handler");
}

void soc_interrupt_init(void)
{
	/* Ensure interrupts of soc are disabled at default */
	for (int i = 0; i < IT8XXX2_INTC_GROUP_CNT; i++) {
		IT8XXX2_INTC_IER(i) = 0;
	}

	/*
	 * WORKAROUND: In the it8xxx2 chip, the interrupt for INT0 is reserved.
	 * However, in some stress tests, the unhandled IRQ0 issue occurs.
	 * To prevent the system from going directly into kernel panic, we
	 * implemented a workaround by registering interrupt number 0 and doing
	 * nothing in the IRQ0 handler. The side effect of this solution is
	 * that when IRQ0 is triggered, it will take some time to execute the
	 * routine. There is no need to worry about missing interrupts because
	 * each IRQ's ISR is write-clear, and if the status is not cleared, it
	 * will continue to trigger.
	 *
	 * NOTE: After this workaround is merged, we will then find out under
	 * what circumstances the situation can be reproduced and fix it, and
	 * then remove the workaround.
	 */
	IRQ_CONNECT(0, 0, intc_irq0_handler, 0, 0);

	/* Enable M-mode external interrupt */
	csr_set(mie, MIP_MEIP);
}

/*
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Platform Level Interrupt Controller (PLIC) driver for Gotham SoC
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <soc.h>
#include <sw_isr_table.h>

#define PLIC_GOTHAM_IRQS        (CONFIG_NUM_IRQS - RISCV_MAX_GENERIC_IRQ)
#define PLIC_GOTHAM_PRI_BITS	3

static int save_irq;

/**
 *
 * @brief Enable a riscv PLIC-specific interrupt line
 *
 * This routine enables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_enable is called by SOC_FAMILY_RISCV_PRIVILEGE
 * _arch_irq_enable function to enable external interrupts for
 * IRQS > RISCV_MAX_GENERIC_IRQ, whenever CONFIG_RISCV_HAS_PLIC
 * variable is set.
 * @param irq IRQ number to enable
 *
 * @return N/A
 */
void riscv_plic_irq_enable(u32_t irq)
{
	u32_t key;
	u32_t gotham_irq = irq - RISCV_MAX_GENERIC_IRQ;
	volatile u32_t *en =
		(volatile u32_t *)GOTHAM_PLIC_REG_IRQ_EN;

	key = irq_lock();
	*en |= (1 << (gotham_irq - 1));
	irq_unlock(key);
}

/**
 *
 * @brief Disable a riscv PLIC-specific interrupt line
 *
 * This routine disables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_disable is called by SOC_FAMILY_RISCV_PRIVILEGE
 * _arch_irq_disable function to disable external interrupts, for
 * IRQS > RISCV_MAX_GENERIC_IRQ, whenever CONFIG_RISCV_HAS_PLIC
 * variable is set.
 * @param irq IRQ number to disable
 *
 * @return N/A
 */
void riscv_plic_irq_disable(u32_t irq)
{
	u32_t key;
	u32_t gotham_irq = irq - RISCV_MAX_GENERIC_IRQ;
	volatile u32_t *en =
		(volatile u32_t *)GOTHAM_PLIC_REG_IRQ_EN;

	key = irq_lock();
	*en &= ~(1 << (gotham_irq - 1));
	irq_unlock(key);
}

/**
 *
 * @brief Check if a riscv PLIC-specific interrupt line is enabled
 *
 * This routine checks if a RISCV PLIC-specific interrupt line is enabled.
 * @param irq IRQ number to check
 *
 * @return 1 or 0
 */
int riscv_plic_irq_is_enabled(u32_t irq)
{
	volatile u32_t *en =
		(volatile u32_t *)GOTHAM_PLIC_REG_IRQ_EN;
	u32_t gotham_irq = irq - RISCV_MAX_GENERIC_IRQ;

	return !!(*en & (1 << (gotham_irq - 1)));
}

/**
 *
 * @brief Set priority of a riscv PLIC-specific interrupt line
 *
 * This routine set the priority of a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_set_prio is called by riscv32 _ARCH_IRQ_CONNECT to set
 * the priority of an interrupt whenever CONFIG_RISCV_HAS_PLIC variable is set.
 * @param irq IRQ number for which to set priority
 *
 * @return N/A
 */
void riscv_plic_set_priority(u32_t irq, u32_t priority)
{
	volatile u32_t *pri =
		(volatile u32_t *)GOTHAM_PLIC_REG_PRI;
	u32_t gotham_irq;
	
	/* Can set priority only for PLIC-specific interrupt line */
	if (irq <= RISCV_MAX_GENERIC_IRQ)
		return;

	if (priority > GOTHAM_PLIC_MAX_PRIORITY)
		priority = GOTHAM_PLIC_MAX_PRIORITY;

	gotham_irq = irq - RISCV_MAX_GENERIC_IRQ;
	*pri |= (priority << ((gotham_irq - 1) * 3));
}

/**
 *
 * @brief Get riscv PLIC-specific interrupt line causing an interrupt
 *
 * This routine returns the RISCV PLIC-specific interrupt line causing an
 * interrupt.
 * @param irq IRQ number for which to set priority
 *
 * @return N/A
 */
int riscv_plic_get_irq(void)
{
	return save_irq;
}

static void plic_gotham_irq_handler(void *arg)
{
	volatile u32_t *id = (volatile u32_t *)GOTHAM_PLIC_REG_ID;
	u32_t irq;
	struct _isr_table_entry *ite;

	/* Get the IRQ number generating the interrupt */
	irq = *id;

	/*
	 * Save IRQ in save_irq. To be used, if need be, by
	 * subsequent handlers registered in the _sw_isr_table table
	 * as the ID register will get over-written at the end of ISR
	 */
	save_irq = irq;

	/*
	 * If the IRQ is out of range, call _irq_spurious.
	 * A call to _irq_spurious will not return.
	 */
	if (irq == 0 || irq >= PLIC_GOTHAM_IRQS)
		_irq_spurious(NULL);

	irq += RISCV_MAX_GENERIC_IRQ;

	/* Call the corresponding IRQ handler in _sw_isr_table */
	ite = (struct _isr_table_entry *)&_sw_isr_table[irq];
	ite->isr(ite->arg);

	/*
	 * Write to ID register to indicate to
	 * PLIC controller that the IRQ has been handled.
	 */
	*id = save_irq;
}

/**
 *
 * @brief Initialize the GOTHAM Platform Level Interrupt Controller
 * @return N/A
 */
static int plic_gotham_init(struct device *dev)
{
	ARG_UNUSED(dev);

	volatile u32_t *en =
		(volatile u32_t *)GOTHAM_PLIC_REG_IRQ_EN;
	volatile u32_t *pri =
		(volatile u32_t *)GOTHAM_PLIC_REG_PRI;
	volatile u32_t *pri_thres =
		(volatile u32_t *)GOTHAM_PLIC_REG_THRES;
	int i;

	/* 
	 * Disable all PLIC specific interrupts
	 * Each interrupt source occupies a specific bit in Interrupt Enable
	 * (IE) register.
	 */
	*en = 0;

	/* Set priority of each interrupt line to 1 initially */
	*pri = 0;
	for (i = 0; i < PLIC_GOTHAM_IRQS; i++)
		*pri |= (1 << (i * PLIC_GOTHAM_PRI_BITS));
	
	/* Set threshold priority to 0 */
	*pri_thres = 0;

	/* Setup IRQ handler for PLIC driver */
	IRQ_CONNECT(RISCV_MACHINE_EXT_IRQ,
		    0,
		    plic_gotham_irq_handler,
		    NULL,
		    0);

	/* Enable IRQ for PLIC driver */
	irq_enable(RISCV_MACHINE_EXT_IRQ);

	return 0;
}

SYS_INIT(plic_gotham_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifive_plic_1_0_0

/**
 * @brief Platform Level Interrupt Controller (PLIC) driver
 *        for RISC-V processors
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <soc.h>

#include <zephyr/sw_isr_table.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/irq.h>

#define PLIC_BASE_ADDR(n) DT_INST_REG_ADDR(n)
/*
 * These registers' offset are defined in the RISCV PLIC specs, see:
 * https://github.com/riscv/riscv-plic-spec
 */
#define PLIC_REG_PRIO_OFFSET 0x0
#define PLIC_REG_IRQ_EN_OFFSET 0x2000
#define PLIC_REG_REGS_OFFSET 0x200000
/*
 * Trigger type is mentioned, but not defined in the RISCV PLIC specs.
 * However, it is defined and supported by at least the Andes & Telink datasheet, and supported
 * in Linux's SiFive PLIC driver
 */
#define PLIC_REG_TRIG_TYPE_OFFSET 0x1080

#define PLIC_MAX_PRIO	DT_INST_PROP(0, riscv_max_priority)
#define PLIC_PRIO	(PLIC_BASE_ADDR(0) + PLIC_REG_PRIO_OFFSET)
#define PLIC_IRQ_EN	(PLIC_BASE_ADDR(0) + PLIC_REG_IRQ_EN_OFFSET)
#define PLIC_REG	(PLIC_BASE_ADDR(0) + PLIC_REG_REGS_OFFSET)

#define PLIC_IRQS        (CONFIG_NUM_IRQS - CONFIG_2ND_LVL_ISR_TBL_OFFSET)
#define PLIC_EN_SIZE     ((PLIC_IRQS >> 5) + 1)

#define PLIC_EDGE_TRIG_TYPE (PLIC_BASE_ADDR(0) + PLIC_REG_TRIG_TYPE_OFFSET)
#define PLIC_EDGE_TRIG_SHIFT  5

struct plic_regs_t {
	uint32_t threshold_prio;
	uint32_t claim_complete;
};

static int save_irq;

/**
 * @brief return edge irq value or zero
 *
 * In the event edge irq is enable this will return the trigger
 * value of the irq. In the event edge irq is not supported this
 * routine will return 0
 *
 * @param irq IRQ number to add to the trigger
 *
 * @return irq value when enabled 0 otherwise
 */
static int riscv_plic_is_edge_irq(uint32_t irq)
{
	volatile uint32_t *trig = (volatile uint32_t *)PLIC_EDGE_TRIG_TYPE;

	trig += (irq >> PLIC_EDGE_TRIG_SHIFT);
	return *trig & BIT(irq);
}

/**
 * @brief Enable a riscv PLIC-specific interrupt line
 *
 * This routine enables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_enable is called by SOC_FAMILY_RISCV_PRIVILEGED
 * arch_irq_enable function to enable external interrupts for
 * IRQS level == 2, whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number to enable
 */
void riscv_plic_irq_enable(uint32_t irq)
{
	uint32_t key;
	volatile uint32_t *en = (volatile uint32_t *)PLIC_IRQ_EN;

	key = irq_lock();
	en += (irq >> 5);
	*en |= (1 << (irq & 31));
	irq_unlock(key);
}

/**
 * @brief Disable a riscv PLIC-specific interrupt line
 *
 * This routine disables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_disable is called by SOC_FAMILY_RISCV_PRIVILEGED
 * arch_irq_disable function to disable external interrupts, for
 * IRQS level == 2, whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number to disable
 */
void riscv_plic_irq_disable(uint32_t irq)
{
	uint32_t key;
	volatile uint32_t *en = (volatile uint32_t *)PLIC_IRQ_EN;

	key = irq_lock();
	en += (irq >> 5);
	*en &= ~(1 << (irq & 31));
	irq_unlock(key);
}

/**
 * @brief Check if a riscv PLIC-specific interrupt line is enabled
 *
 * This routine checks if a RISCV PLIC-specific interrupt line is enabled.
 * @param irq IRQ number to check
 *
 * @return 1 or 0
 */
int riscv_plic_irq_is_enabled(uint32_t irq)
{
	volatile uint32_t *en = (volatile uint32_t *)PLIC_IRQ_EN;

	en += (irq >> 5);
	return !!(*en & (1 << (irq & 31)));
}

/**
 * @brief Set priority of a riscv PLIC-specific interrupt line
 *
 * This routine set the priority of a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_set_prio is called by riscv arch_irq_priority_set to set
 * the priority of an interrupt whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number for which to set priority
 * @param priority Priority of IRQ to set to
 */
void riscv_plic_set_priority(uint32_t irq, uint32_t priority)
{
	volatile uint32_t *prio = (volatile uint32_t *)PLIC_PRIO;

	if (priority > PLIC_MAX_PRIO)
		priority = PLIC_MAX_PRIO;

	prio += irq;
	*prio = priority;
}

/**
 * @brief Get riscv PLIC-specific interrupt line causing an interrupt
 *
 * This routine returns the RISCV PLIC-specific interrupt line causing an
 * interrupt.
 *
 * @return PLIC-specific interrupt line causing an interrupt.
 */
int riscv_plic_get_irq(void)
{
	return save_irq;
}

static void plic_irq_handler(const void *arg)
{
	volatile struct plic_regs_t *regs =
	    (volatile struct plic_regs_t *) PLIC_REG;

	uint32_t irq;
	struct _isr_table_entry *ite;
	int edge_irq;

	/* Get the IRQ number generating the interrupt */
	irq = regs->claim_complete;

	/*
	 * Save IRQ in save_irq. To be used, if need be, by
	 * subsequent handlers registered in the _sw_isr_table table,
	 * as IRQ number held by the claim_complete register is
	 * cleared upon read.
	 */
	save_irq = irq;

	/*
	 * If the IRQ is out of range, call z_irq_spurious.
	 * A call to z_irq_spurious will not return.
	 */
	if (irq == 0U || irq >= PLIC_IRQS)
		z_irq_spurious(NULL);

	edge_irq = riscv_plic_is_edge_irq(irq);

	/*
	 * For edge triggered interrupts, write to the claim_complete register
	 * to indicate to the PLIC controller that the IRQ has been handled
	 * for edge triggered interrupts.
	 */
	if (edge_irq)
		regs->claim_complete = save_irq;

	irq += CONFIG_2ND_LVL_ISR_TBL_OFFSET;

	/* Call the corresponding IRQ handler in _sw_isr_table */
	ite = (struct _isr_table_entry *)&_sw_isr_table[irq];
	ite->isr(ite->arg);

	/*
	 * Write to claim_complete register to indicate to
	 * PLIC controller that the IRQ has been handled
	 * for level triggered interrupts.
	 */
	if (!edge_irq)
		regs->claim_complete = save_irq;
}

/**
 * @brief Initialize the Platform Level Interrupt Controller
 *
 * @retval 0 on success.
 */
static int plic_init(const struct device *dev)
{

	volatile uint32_t *en = (volatile uint32_t *)PLIC_IRQ_EN;
	volatile uint32_t *prio = (volatile uint32_t *)PLIC_PRIO;
	volatile struct plic_regs_t *regs =
	    (volatile struct plic_regs_t *)PLIC_REG;
	int i;

	/* Ensure that all interrupts are disabled initially */
	for (i = 0; i < PLIC_EN_SIZE; i++) {
		*en = 0U;
		en++;
	}

	/* Set priority of each interrupt line to 0 initially */
	for (i = 0; i < PLIC_IRQS; i++) {
		*prio = 0U;
		prio++;
	}

	/* Set threshold priority to 0 */
	regs->threshold_prio = 0U;

	/* Setup IRQ handler for PLIC driver */
	IRQ_CONNECT(RISCV_MACHINE_EXT_IRQ,
		    0,
		    plic_irq_handler,
		    NULL,
		    0);

	/* Enable IRQ for PLIC driver */
	irq_enable(RISCV_MACHINE_EXT_IRQ);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, plic_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

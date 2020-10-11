/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *               2020 Jim Shu <cwshu@andestech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifive_plic_1_0_0

/**
 * @brief Platform Level Interrupt Controller (PLIC) driver
 *        for RISC-V processors
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <soc.h>

#include <sw_isr_table.h>

#define PLIC_MAX_PRIO	DT_INST_PROP(0, riscv_max_priority)
#define PLIC_PRIO	DT_INST_REG_ADDR_BY_NAME(0, prio)
#define PLIC_IRQ_EN	DT_INST_REG_ADDR_BY_NAME(0, irq_en)
#define PLIC_REG	DT_INST_REG_ADDR_BY_NAME(0, reg)

#define PLIC_IRQS        (CONFIG_NUM_IRQS - CONFIG_2ND_LVL_ISR_TBL_OFFSET)
#define PLIC_EN_SIZE     ((PLIC_IRQS >> 5) + 1)

/*
 * In PLIC, interrupt target (or hart context) is a given privilege mode
 * on a given hart. How PLIC organizes interrupt target is based on
 * vendor implementation.
 *
 * For example, in QEMU virt machine:
 *   - target 0, 2, 4, ... are M-mode hart 0, 1, 2, 3 ...
 *   - target 1, 3, 5, ... are S-mode hart 0, 1, 2, 3 ...
 *
 * Currently, we assume next hart in M-mode is the multiple of next
 * interrupt target.
 */
#ifdef CONFIG_SMP
# define PLIC_EN_PER_TARGET      0x80
# define PLIC_REG_PER_TARGET     0x1000
# define PLIC_EN_PER_HART        (PLIC_EN_PER_TARGET * CONFIG_PLIC_TARGET_INCR_OF_NEXT_HART)
# define PLIC_REG_PER_HART       (PLIC_REG_PER_TARGET * CONFIG_PLIC_TARGET_INCR_OF_NEXT_HART)
#else
/* In UP mode, these macro are used but meaningless */
# define PLIC_EN_PER_HART        0
# define PLIC_REG_PER_HART       0
#endif

#ifdef CONFIG_SMP
# define CORES_NUM   CONFIG_MP_NUM_CPUS
#else
# define CORES_NUM   1
#endif

static struct k_spinlock lock;

struct plic_regs_t {
	uint32_t threshold_prio;
	uint32_t claim_complete;
};

static int save_irq;

/**
 *
 * @brief Enable a riscv PLIC-specific interrupt line
 *
 * This routine enables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_enable is called by SOC_FAMILY_RISCV_PRIVILEGE
 * arch_irq_enable function to enable external interrupts for
 * IRQS level == 2, whenever CONFIG_RISCV_HAS_PLIC variable is set.
 * @param irq IRQ number to enable
 *
 * @return N/A
 */
void riscv_plic_irq_enable(uint32_t irq)
{
	volatile uint32_t *en = (volatile uint32_t *)PLIC_IRQ_EN;
	en += (irq >> 5);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* enable irq on each core in SMP */
	for(int i = 0; i < CORES_NUM; i++)
	{
		*en |= (1 << (irq & 31));
		en += (PLIC_EN_PER_HART >> 2);
	}

	k_spin_unlock(&lock, key);
}

/**
 *
 * @brief Disable a riscv PLIC-specific interrupt line
 *
 * This routine disables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_disable is called by SOC_FAMILY_RISCV_PRIVILEGE
 * arch_irq_disable function to disable external interrupts, for
 * IRQS level == 2, whenever CONFIG_RISCV_HAS_PLIC variable is set.
 * @param irq IRQ number to disable
 *
 * @return N/A
 */
void riscv_plic_irq_disable(uint32_t irq)
{
	volatile uint32_t *en = (volatile uint32_t *)PLIC_IRQ_EN;
	en += (irq >> 5);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* disable irq on each core in SMP */
	for(int i = 0; i < CORES_NUM; i++)
	{
		*en &= ~(1 << (irq & 31));
		en += (PLIC_EN_PER_HART >> 2);
	}

	k_spin_unlock(&lock, key);
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
int riscv_plic_irq_is_enabled(uint32_t irq)
{
	volatile uint32_t *en = (volatile uint32_t *)PLIC_IRQ_EN;
	int is_enabled = 1;
	en += (irq >> 5);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Note: In SMP, irq is enabled if irq of each core is all enabled */
	for(int i = 0; i < CORES_NUM; i++)
	{
		is_enabled &= !!(*en & (1 << (irq & 31)));
		en += (PLIC_EN_PER_HART >> 2);
	}

	k_spin_unlock(&lock, key);

	return is_enabled;
}

/**
 *
 * @brief Set priority of a riscv PLIC-specific interrupt line
 *
 * This routine set the priority of a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_set_prio is called by riscv arch_irq_priority_set to set
 * the priority of an interrupt whenever CONFIG_RISCV_HAS_PLIC variable is set.
 * @param irq IRQ number for which to set priority
 *
 * @return N/A
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

static void plic_irq_handler(const void *arg)
{
	volatile struct plic_regs_t *regs = (volatile struct plic_regs_t *)
		INT_TO_POINTER(PLIC_REG + (_current_cpu->id * PLIC_REG_PER_HART));

	uint32_t irq;
	struct _isr_table_entry *ite;

	/* Get the IRQ number generating the interrupt */
	irq = regs->claim_complete;

	/*
	 * Save IRQ in save_irq. To be used, if need be, by
	 * subsequent handlers registered in the _sw_isr_table table,
	 * as IRQ number held by the claim_complete register is
	 * cleared upon read.
	 */
	save_irq = irq;

#ifdef CONFIG_SMP
	/*
	 * FIXME: In SMP, sometimes CPUs may claim the IRQ with ID == 0.
	 * Claim ID 0 in PLIC means that there isn't any pending IRQ now.
	 * It isn't clear that it's a SW bug or just a common behavior
	 * in SMP, but in the testing we don't lose handling any IRQ.
	 */
	if (irq == 0U)
		return;
#endif

	/*
	 * If the IRQ is out of range, call z_irq_spurious.
	 * A call to z_irq_spurious will not return.
	 */
	if (irq == 0U || irq >= PLIC_IRQS)
		z_irq_spurious(NULL);

	irq += CONFIG_2ND_LVL_ISR_TBL_OFFSET;

	/* Call the corresponding IRQ handler in _sw_isr_table */
	ite = (struct _isr_table_entry *)&_sw_isr_table[irq];
	ite->isr(ite->arg);

	/*
	 * Write to claim_complete register to indicate to
	 * PLIC controller that the IRQ has been handled.
	 */
	regs->claim_complete = save_irq;
}

/**
 *
 * @brief Initialize the Platform Level Interrupt Controller
 * @return N/A
 */
static int plic_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	volatile uint32_t *en = (volatile uint32_t *)PLIC_IRQ_EN;
	volatile uint32_t *prio = (volatile uint32_t *)PLIC_PRIO;
	volatile struct plic_regs_t *regs =
	    (volatile struct plic_regs_t *)PLIC_REG;
	int i, j;

	/* Ensure that all interrupts are disabled initially */
	for (i = 0; i < CORES_NUM; i++) {
		volatile uint32_t *en_cpu = en;
		for (j = 0; j < PLIC_EN_SIZE; j++) {
			*en_cpu = 0U;
			en_cpu++;
		}

		en += (PLIC_EN_PER_HART >> 2);
	}

	/* Set priority of each interrupt line to 0 initially */
	for (i = 0; i < PLIC_IRQS; i++) {
		*prio = 0U;
		prio++;
	}

	/* Set threshold priority to 0 */
	regs->threshold_prio = 0U;

	for (i = 0; i < CORES_NUM ; i++) {
		regs->threshold_prio = 0U;
		regs = (volatile struct plic_regs_t *)
			((uint8_t *)regs + PLIC_REG_PER_HART);
	}

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

SYS_INIT(plic_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

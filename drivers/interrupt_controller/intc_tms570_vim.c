/*
 * Copyright (C) 2025 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <string.h>

#define DT_DRV_COMPAT ti_tms570_vim

#define DRV_CONTROL_REG		DT_INST_REG_ADDR_BY_IDX(0, 0)
#define DRV_ECC_REG		DT_INST_REG_ADDR_BY_IDX(0, 1)
#define DRV_VIMRAM_REG		DT_INST_REG_ADDR_BY_IDX(0, 2)
#define DRV_VIMRAM_REG_SIZE	DT_INST_REG_SIZE_BY_IDX(0, 2)

/* control registers */
#define VIM_REG_IRQINDEX	(DRV_CONTROL_REG + 0x00)
#define VIM_REG_FIQINDEX	(DRV_CONTROL_REG + 0x04)
#define VIM_REG_REQMASKSET_0	(DRV_CONTROL_REG + 0x30) /* 0,1,2,3 in sequence, each is a 4 byte register*/
#define VIM_REG_REQMASKCLR_0	(DRV_CONTROL_REG + 0x40) /* 0,1,2,3 in sequence, each is a 4 byte register*/

/* ECC related registers */
#define VIM_ECC_CTL		(DRV_ECC_REG + 0xF0)

#define REQUMASK_IRQ_PER_REG	(32u)

static inline void set_reqmask_bit(unsigned int irq, uintptr_t reg_0_addr)
{
	sys_write32(1 << (irq % REQUMASK_IRQ_PER_REG),
		  reg_0_addr + (irq / REQUMASK_IRQ_PER_REG) * sizeof(uint32_t));
}

static inline int get_reqmask_bit(unsigned int irq, uintptr_t reg_0_addr)
{
	return sys_read32(reg_0_addr + (irq / REQUMASK_IRQ_PER_REG) * sizeof(uint32_t));
}

/* count of number of times the phantom interrupt happened */
unsigned int nr_phantom_isr;

static void phantom_isr(void)
{
	/**
	 * we don't want this to call the z_spurious_irq because we have seen
	 * phantom irq happen even though we don't expect it to happen.
	 */
	nr_phantom_isr++;
}

#if defined(CONFIG_RUNTIME_NMI)
static void tms570_nmi_handler(void)
{
	while (1) {
		/* forever */
	}
}
#endif

/**
 * @brief Get active interrupt ID (IRQ only)
 *
 * @return Returns the ID of an active interrupt
 */
unsigned int z_soc_irq_get_active(void)
{
	unsigned int irq_idx;

	/* a 0 means phantom ISR, channel 0 starts from index 1 */
	irq_idx = sys_read32(VIM_REG_IRQINDEX);
	if (irq_idx > 0) {
		z_soc_irq_disable(irq_idx - 1);
		return irq_idx - 1;

	} else {
		phantom_isr();
	}

	return (CONFIG_NUM_IRQS + 1);
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	/**
	 * not supported, all IRQ sources generate IRQ, instead of FIQ,
	 * and with the default priority.
	 */
}

void z_soc_irq_enable(unsigned int irq)
{
	set_reqmask_bit(irq, VIM_REG_REQMASKSET_0);
}

void z_soc_irq_disable(unsigned int irq)
{
	set_reqmask_bit(irq, VIM_REG_REQMASKCLR_0);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	return get_reqmask_bit(irq, VIM_REG_REQMASKSET_0);
}

/**
 * @brief Signal end-of-interrupt
 *
 * @param irq interrupt ID
 */
void z_soc_irq_eoi(unsigned int irq)
{
	z_soc_irq_enable(irq);
}

void z_soc_irq_init(void)
{
	/**
	 * ref. SPNA218.pdf
	 * We are implementing what is referred to as "Legacy ARM7 Interrupts".
	 * We do not use the VIM_RAM at all.
	 * Sequence is like this:
	 * 1. Interrupt request happens
	 * 2. Exception vector 0x18 (IRQ) or 0x1C (FIQ) is taken
	 *      - in case of IRQ "ldr pc, =_isr_wrapper"
	 *      - in case of FIQ "ldr pc, =z_arm_nmi"
	 * 3. _isr_wrapper uses z_soc_irq_get_active to get index into
	 *    _sw_isr_table for arg and ISR handler
	 * 4. run ISR handler
	 *
	 * Drivers attach interrupts using IRQ_CONNECT/IRQ_DIRECT_CONNECT like:
	 * 	IRQ_CONNECT(irqnum, irqnum, z_irq_spurious, NULL, 0);
	 */

	/* Errata VIM#28 Workaround: Disable Single Bit error correction */
	sys_write32((0xAU << 0U) | (0x5U << 16U), VIM_ECC_CTL);

	/*
	   spent a long time debugging a ESM2.25 NMI fault which is
	   "CCM-R5F VIM compare error". Could not find cause but this
	   seems to have made it stop.
	*/
	memset((void *)DRV_VIMRAM_REG, 0, DRV_VIMRAM_REG_SIZE);

#if defined(CONFIG_RUNTIME_NMI)
	z_arm_nmi_set_handler(tms570_nmi_handler);
#endif

	/* enable interrupt */
	arch_irq_unlock(0);
}

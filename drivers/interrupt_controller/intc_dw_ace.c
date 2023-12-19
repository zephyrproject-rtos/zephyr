/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/arch/xtensa/irq.h>
#ifdef CONFIG_DYNAMIC_INTERRUPTS
#include <zephyr/sw_isr_table.h>
#endif
#include <zephyr/drivers/interrupt_controller/dw_ace.h>
#include <soc.h>
#include <adsp_interrupt.h>
#include <zephyr/irq.h>
#include "intc_dw.h"

/* ACE device interrupts are all packed into a single line on Xtensa's
 * architectural IRQ 4 (see below), run by a Designware interrupt
 * controller with 28 lines instantiated.  They get numbered
 * immediately after the Xtensa interrupt space in the numbering
 * (i.e. interrupts 0-31 are Xtensa IRQs, 32 represents DW input 0,
 * etc...).
 *
 * That IRQ 4 indeed has an interrupt type of "EXTERN_LEVEL" and an
 * interrupt level of 2.  The CPU has a level 1 external interrupt on
 * IRQ 1 and a level 3 on IRQ 6, but nothing seems wired there.  Note
 * that this level 2 ISR is also shared with the CCOUNT timer on IRQ3.
 * This interrupt is a very busy place!
 *
 * But, because there can never be a situation where all interrupts on
 * the Synopsys controller are disabled (such a system would halt
 * forever if it reached idle!), we at least can take advantage to
 * implement a simplified masking architecture.  Xtensa INTENABLE
 * always has the line active, and we do all masking of external
 * interrupts on the single controller.
 *
 * Finally: note that there is an extra layer of masking on ACE.  The
 * ACE_DINT registers provide separately maskable interrupt delivery
 * for each core, and with some devices for different internal
 * interrupt sources.  Responsibility for these mask bits is left with
 * the driver.
 *
 * Thus, the masking architecture picked here is:
 *
 * + Drivers manage ACE_DINT themselves, as there are device-specific
 *   mask indexes that only the driver can interpret.  If
 *   core-asymmetric interrupt routing needs to happen, it happens
 *   here.
 *
 * + The DW layer is en/disabled uniformly across all cores.  This is
 *   the layer toggled by arch_irq_en/disable().
 *
 * + Index 4 in the INTENABLE SR is set at core startup and stays
 *   enabled always.
 */

/* ACE also has per-core instantiations of a Synopsys interrupt
 * controller.  These inputs (with the same indices as ACE_INTL_*
 * above) are downstream of the DINT layer, and must be independently
 * masked/enabled.  The core Zephyr intc_dw driver unfortunately
 * doesn't understand this kind of MP implementation.  Note also that
 * as instantiated (there are only 28 sources), the high 32 bit
 * registers don't exist and aren't named here.  Access via e.g.:
 *
 *     ACE_INTC[core_id].irq_inten_l |= interrupt_bit;
 */

#define ACE_INTC ((volatile struct dw_ictl_registers *)DT_REG_ADDR(DT_NODELABEL(ace_intc)))

static inline bool is_dw_irq(uint32_t irq)
{
	if (((irq & XTENSA_IRQ_NUM_MASK) == ACE_INTC_IRQ)
	    && ((irq & ~XTENSA_IRQ_NUM_MASK) != 0)) {
		return true;
	}

	return false;
}

void dw_ace_irq_enable(const struct device *dev, uint32_t irq)
{
	ARG_UNUSED(dev);

	if (is_dw_irq(irq)) {
		unsigned int num_cpus = arch_num_cpus();

		for (int i = 0; i < num_cpus; i++) {
			ACE_INTC[i].irq_inten_l |= BIT(ACE_IRQ_FROM_ZEPHYR(irq));
			ACE_INTC[i].irq_intmask_l &= ~BIT(ACE_IRQ_FROM_ZEPHYR(irq));
		}
	} else if ((irq & ~XTENSA_IRQ_NUM_MASK) == 0U) {
		xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));
	}
}

void dw_ace_irq_disable(const struct device *dev, uint32_t irq)
{
	ARG_UNUSED(dev);

	if (is_dw_irq(irq)) {
		unsigned int num_cpus = arch_num_cpus();

		for (int i = 0; i < num_cpus; i++) {
			ACE_INTC[i].irq_inten_l &= ~BIT(ACE_IRQ_FROM_ZEPHYR(irq));
			ACE_INTC[i].irq_intmask_l |= BIT(ACE_IRQ_FROM_ZEPHYR(irq));
		}
	} else if ((irq & ~XTENSA_IRQ_NUM_MASK) == 0U) {
		xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
	}
}

int dw_ace_irq_is_enabled(const struct device *dev, unsigned int irq)
{
	ARG_UNUSED(dev);

	if (is_dw_irq(irq)) {
		return ACE_INTC[0].irq_inten_l & BIT(ACE_IRQ_FROM_ZEPHYR(irq));
	} else if ((irq & ~XTENSA_IRQ_NUM_MASK) == 0U) {
		return xtensa_irq_is_enabled(XTENSA_IRQ_NUMBER(irq));
	}

	return false;
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int dw_ace_irq_connect_dynamic(const struct device *dev, unsigned int irq,
				   unsigned int priority,
				   void (*routine)(const void *parameter),
				   const void *parameter, uint32_t flags)
{
	/* Simple architecture means that the Zephyr irq number and
	 * the index into the ISR table are identical.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);
	z_isr_install(irq, routine, parameter);
	return irq;
}
#endif

static void dwint_isr(const void *arg)
{
	uint32_t fs = ACE_INTC[arch_proc_id()].irq_finalstatus_l;

	while (fs) {
		uint32_t bit = find_lsb_set(fs) - 1;
		uint32_t offset = CONFIG_2ND_LVL_ISR_TBL_OFFSET + bit;
		struct _isr_table_entry *ent = &_sw_isr_table[offset];

		fs &= ~BIT(bit);
		ent->isr(ent->arg);
	}
}

static int dw_ace_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(ACE_INTC_IRQ, 0, dwint_isr, 0, 0);
	xtensa_irq_enable(ACE_INTC_IRQ);

	return 0;
}

static const struct dw_ace_v1_ictl_driver_api dw_ictl_ace_v1x_apis = {
	.intr_enable = dw_ace_irq_enable,
	.intr_disable = dw_ace_irq_disable,
	.intr_is_enabled = dw_ace_irq_is_enabled,
#ifdef CONFIG_DYNAMIC_INTERRUPTS
	.intr_connect_dynamic = dw_ace_irq_connect_dynamic,
#endif
};

DEVICE_DT_DEFINE(DT_NODELABEL(ace_intc), dw_ace_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		 &dw_ictl_ace_v1x_apis);

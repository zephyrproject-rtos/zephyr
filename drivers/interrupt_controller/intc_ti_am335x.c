/*
 * Copyright (c) 2024 Abe Kohandel <abe.kohandel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TI AM335x series interrupt controller driver
 */

#include "zephyr/arch/arm/cortex_a_r/sys_io.h"
#include "zephyr/devicetree.h"
#include "zephyr/sys/util_macro.h"
#include "zephyr/toolchain.h"
#include "intc_ti_am335x.h"

#define DT_DRV_COMPAT ti_am335x_intc

#define INTC_SIR_IRQ_OFFSET      0x0040
#define INTC_CONTROL_OFFSET      0x0048
#define INTC_MIR0_OFFSET         0x0084
#define INTC_MIR_CLEAR0_OFFSET   0x0088
#define INTC_MIR_SET0_OFFSET     0x008c
#define INTC_PENDING_IRQ0_OFFSET 0x0098
#define INTC_ILR_0_OFFSET        0x0100

#define INTC_STRIDE      0x20
#define INTC_IRQ_PER_GRP 32

#define INTC_IRQ_GRP(irq) ((irq) / INTC_IRQ_PER_GRP)
#define INTC_IRQ_IDX(irq) ((irq) % INTC_IRQ_PER_GRP)

#define INTC_BASE DT_INST_REG_ADDR(0)

#define INTC_SIR_IRQ          (INTC_BASE + INTC_SIR_IRQ_OFFSET)
#define INTC_CTRL             (INTC_BASE + INTC_CONTROL_OFFSET)
#define INTC_MIR(grp)         (INTC_BASE + INTC_MIR0_OFFSET + (grp) * INTC_STRIDE)
#define INTC_MIR_CLR(grp)     (INTC_BASE + INTC_MIR_CLEAR0_OFFSET + (grp) * INTC_STRIDE)
#define INTC_MIR_SET(grp)     (INTC_BASE + INTC_MIR_SET0_OFFSET + (grp) * INTC_STRIDE)
#define INTC_PENDING_IRQ(grp) (INTC_BASE + INTC_PENDING_IRQ0_OFFSET + (grp) * INTC_STRIDE)
#define INTC_PRIO(irq)        (INTC_BASE + INTC_ILR_0_OFFSET + (irq) * 4)

void intc_ti_am335x_irq_init(void)
{
	/* mask all interrupts */
	for (int grp = 0; grp < CONFIG_NUM_IRQS / INTC_IRQ_PER_GRP; grp++) {
		sys_write32(~0U, INTC_MIR_SET(grp));
	}
}

void intc_ti_am335x_irq_enable(unsigned int irq)
{
	sys_write32(BIT(INTC_IRQ_IDX(irq)), INTC_MIR_CLR(INTC_IRQ_GRP(irq)));
}

void intc_ti_am335x_irq_disable(unsigned int irq)
{
	sys_write32(BIT(INTC_IRQ_IDX(irq)), INTC_MIR_SET(INTC_IRQ_GRP(irq)));
}

int intc_ti_am335x_irq_is_enabled(unsigned int irq)
{
	return sys_read32(INTC_MIR(INTC_IRQ_GRP(irq))) & BIT(INTC_IRQ_IDX(irq));
}

void intc_ti_am335x_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	ARG_UNUSED(flags);

	sys_write32(prio << 2, INTC_PRIO(irq));
}

unsigned int intc_ti_am335x_irq_get_active(void)
{
	int irq;

	/* get active IRQ */
	irq = sys_read32(INTC_SIR_IRQ) & BIT_MASK(7);

	/* mask active IRQ */
	intc_ti_am335x_irq_disable(irq);

	/* request new IRQ generation */
	sys_write32(1, INTC_CTRL);

	return irq;
}

void intc_ti_am335x_irq_eoi(unsigned int irq)
{
	intc_ti_am335x_irq_enable(irq);
}

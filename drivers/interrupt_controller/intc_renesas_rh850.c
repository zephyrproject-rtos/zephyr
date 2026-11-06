/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rh850_intc

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_renesas_rh850.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#define INTC1_BASE DT_INST_REG_ADDR_BY_NAME(0, intc1)
#define INTC2_BASE DT_INST_REG_ADDR_BY_NAME(0, intc2)

/* Channels 0-31 are in INTC1 of the CPU, the others in INTC2 */
#define INTC1_CHANNELS 32U

/* EIC low and high bytes are written separately so that EIRF is not touched */
#define EIC_EIMK    BIT(7)
#define EIC_EITB    BIT(6)
#define EIC_EIP     GENMASK(3, 0)
#define EIC_HI_EIRF BIT(4)

static mem_addr_t intc_rh850_eic(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "channel %u out of range", irq);

	return ((irq < INTC1_CHANNELS) ? INTC1_BASE : INTC2_BASE) + (2U * irq);
}

void intc_rh850_irq_set_priority(unsigned int irq, unsigned int prio)
{
	mem_addr_t eic = intc_rh850_eic(irq);
	uint8_t lo = sys_read8(eic);

	lo &= (uint8_t)~(EIC_EITB | EIC_EIP);
	lo |= (uint8_t)FIELD_PREP(EIC_EIP, prio);
	sys_write8(lo, eic);
}

void intc_rh850_irq_enable(unsigned int irq)
{
	mem_addr_t eic = intc_rh850_eic(irq);

	sys_write8(sys_read8(eic) & (uint8_t)~EIC_EIMK, eic);
}

void intc_rh850_irq_disable(unsigned int irq)
{
	mem_addr_t eic = intc_rh850_eic(irq);

	sys_write8(sys_read8(eic) | (uint8_t)EIC_EIMK, eic);
}

bool intc_rh850_irq_is_enabled(unsigned int irq)
{
	return (sys_read8(intc_rh850_eic(irq)) & EIC_EIMK) == 0U;
}

void intc_rh850_irq_raise(unsigned int irq)
{
	sys_write8(EIC_HI_EIRF, intc_rh850_eic(irq) + 1U);
}

bool intc_rh850_irq_is_pending(unsigned int irq)
{
	return (sys_read8(intc_rh850_eic(irq) + 1U) & EIC_HI_EIRF) != 0U;
}

void intc_rh850_irq_clear_pending(unsigned int irq)
{
	sys_write8(0, intc_rh850_eic(irq) + 1U);
}

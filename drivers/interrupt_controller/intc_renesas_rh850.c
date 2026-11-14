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

/*
 * With 64 priority levels (INTCFG.EPL = 1) only the 32-bit EEICn registers are
 * used, otherwise only the 16-bit EICn registers. Each byte is written on its
 * own so that EIRF is not touched.
 */
#if CONFIG_NUM_IRQ_PRIO_LEVELS > 16
#define ICR_INTC1_OFFSET 0x200U
#define ICR_INTC2_OFFSET 0x4000U
#define ICR_STRIDE       4U
#define ICR_MASK_BYTE    2U
#define ICR_FLAG_BYTE    3U
#define ICR_EIP          GENMASK(5, 0)
#else
#define ICR_INTC1_OFFSET 0U
#define ICR_INTC2_OFFSET 0U
#define ICR_STRIDE       2U
#define ICR_MASK_BYTE    0U
#define ICR_FLAG_BYTE    1U
#define ICR_EIP          GENMASK(3, 0)
#endif

#define ICR_EIMK BIT(7)
#define ICR_EITB BIT(6)
#define ICR_EIRF BIT(4)

static mem_addr_t intc_rh850_eic(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "channel %u out of range", irq);

	if (irq < INTC1_CHANNELS) {
		return INTC1_BASE + ICR_INTC1_OFFSET + (ICR_STRIDE * irq);
	}

	return INTC2_BASE + ICR_INTC2_OFFSET + (ICR_STRIDE * irq);
}

void intc_rh850_irq_set_priority(unsigned int irq, unsigned int prio)
{
	mem_addr_t eic = intc_rh850_eic(irq);
	uint8_t lo = sys_read8(eic);

	__ASSERT(prio < CONFIG_NUM_IRQ_PRIO_LEVELS, "invalid priority %u for channel %u", prio,
		 irq);

	lo &= (uint8_t)~ICR_EIP;
	lo |= (uint8_t)FIELD_PREP(ICR_EIP, prio);
	if (ICR_MASK_BYTE == 0U) {
		lo &= (uint8_t)~ICR_EITB;
	} else {
		sys_write8(sys_read8(eic + ICR_MASK_BYTE) & (uint8_t)~ICR_EITB,
			   eic + ICR_MASK_BYTE);
	}
	sys_write8(lo, eic);
}

void intc_rh850_irq_enable(unsigned int irq)
{
	mem_addr_t eic = intc_rh850_eic(irq) + ICR_MASK_BYTE;

	sys_write8(sys_read8(eic) & (uint8_t)~ICR_EIMK, eic);
}

void intc_rh850_irq_disable(unsigned int irq)
{
	mem_addr_t eic = intc_rh850_eic(irq) + ICR_MASK_BYTE;

	sys_write8(sys_read8(eic) | (uint8_t)ICR_EIMK, eic);
}

bool intc_rh850_irq_is_enabled(unsigned int irq)
{
	return (sys_read8(intc_rh850_eic(irq) + ICR_MASK_BYTE) & ICR_EIMK) == 0U;
}

void intc_rh850_irq_raise(unsigned int irq)
{
	sys_write8(ICR_EIRF, intc_rh850_eic(irq) + ICR_FLAG_BYTE);
}

bool intc_rh850_irq_is_pending(unsigned int irq)
{
	return (sys_read8(intc_rh850_eic(irq) + ICR_FLAG_BYTE) & ICR_EIRF) != 0U;
}

void intc_rh850_irq_clear_pending(unsigned int irq)
{
	sys_write8(0, intc_rh850_eic(irq) + ICR_FLAG_BYTE);
}

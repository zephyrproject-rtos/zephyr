/*
 * Copyright (c) 2023 - 2024 TOKITTA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_interrupt_controller_unit

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/drivers/interrupt_controller/intc_ra_icu.h>
#include <zephyr/sw_isr_table.h>
#include <errno.h>

#define IELSRn_REG(n) (DT_INST_REG_ADDR(0) + IELSRn_OFFSET + (n * 4))
#define IRQCRi_REG(i) (DT_INST_REG_ADDR(0) + IRQCRi_OFFSET + (i))

#define IRQCRi_IRQMD_POS  0
#define IRQCRi_IRQMD_MASK BIT_MASK(2)
#define IELSRn_IR_POS     16
#define IELSRn_IR_MASK    BIT_MASK(1)

const ra_intr_id_t __weak __intr_id_table[RA_INTR_ID_COUNT] = {};

enum {
	IRQCRi_OFFSET = 0x0,
	IELSRn_OFFSET = 0x300,
};

static inline unsigned int event_to_irq(unsigned int event)
{
	if (event >= RA_INTR_ID_COUNT) {
		return RA_INVALID_INTR_ID;
	}

	return RA_INTR_ID_TO_IRQ(event);
}

void ra_icu_event_enable(unsigned int event)
{
	const unsigned int irq = event_to_irq(event);

	if (irq == RA_INVALID_INTR_ID) {
		return;
	}

	sys_write32(event, IELSRn_REG(irq));
	irq_enable(irq);
}

void ra_icu_event_disable(unsigned int event)
{
	const unsigned int irq = event_to_irq(event);

	if (irq == RA_INVALID_INTR_ID) {
		return;
	}

	irq_disable(irq);
	sys_write32(0, IELSRn_REG(irq));
}

int ra_icu_event_is_enabled(unsigned int event)
{
	const unsigned int irq = event_to_irq(event);

	if (irq == RA_INVALID_INTR_ID) {
		return 0;
	}

	return irq_is_enabled(irq);
}

void ra_icu_event_priority_set(unsigned int event, unsigned int prio, uint32_t flags)
{
	const unsigned int irq = event_to_irq(event);

	if (irq == RA_INVALID_INTR_ID) {
		return;
	}

	z_arm_irq_priority_set(irq, prio, flags);
}

void ra_icu_event_clear_flag(unsigned int event)
{
	const unsigned int irq = event_to_irq(event);

	if (irq == RA_INVALID_INTR_ID) {
		return;
	}

	sys_write32(sys_read32(IELSRn_REG(irq)) & ~BIT(IELSRn_IR_POS), IELSRn_REG(irq));
}

void ra_icu_event_query_config(unsigned int event, uint32_t *intcfg)
{
	const unsigned int extirq = event - 1;

	if (extirq >= 16) {
		return;
	}

	*intcfg = sys_read8(IRQCRi_REG(extirq));
}

void ra_icu_event_configure(unsigned int event, uint32_t intcfg)
{
	const unsigned int extirq = event - 1;

	if (extirq >= 16) {
		return;
	}

	sys_write8((intcfg & IRQCRi_IRQMD_MASK) |
			   (sys_read8(IRQCRi_REG(extirq)) & ~(IRQCRi_IRQMD_MASK)),
		   IRQCRi_REG(extirq));
}

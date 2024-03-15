/*
 * Copyright (c) 2023 TOKITTA Hiroshi <tokita.hiroshi@fujitsu.com>
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

enum {
	IRQCRi_OFFSET = 0x0,
	IELSRn_OFFSET = 0x300,
};

int ra_icu_query_exists_irq(uint32_t event)
{
	for (uint32_t i = 0; i < CONFIG_NUM_IRQS; i++) {
		uint32_t els = sys_read32(IELSRn_REG(i)) & UINT8_MAX;

		if (event == els) {
			return i;
		}
	}

	return -EINVAL;
}

int ra_icu_query_available_irq(uint32_t event)
{
	int irq = -EINVAL;

	if (ra_icu_query_exists_irq(event) > 0) {
		return -EINVAL;
	}

	for (uint32_t i = 0; i < CONFIG_NUM_IRQS; i++) {
		if (_sw_isr_table[i].isr == z_irq_spurious) {
			irq = i;
			break;
		}
	}

	return irq;
}

void ra_icu_clear_int_flag(unsigned int irqn)
{
	uint32_t cfg = sys_read32(IELSRn_REG(irqn));

	sys_write32(cfg & ~BIT(IELSRn_IR_POS), IELSRn_REG(irqn));
}

void ra_icu_query_irq_config(unsigned int irq, uint32_t *intcfg, ra_isr_handler *cb,
			     const void **cbarg)
{
	*intcfg = sys_read32(IELSRn_REG(irq));
	*cb = _sw_isr_table[irq].isr;
	*cbarg = (void *)_sw_isr_table[irq].arg;
}

static void ra_icu_irq_configure(unsigned int irqn, uint32_t intcfg)
{
	uint8_t reg = sys_read8(IRQCRi_REG(irqn)) & ~(IRQCRi_IRQMD_MASK);

	sys_write8(reg | (intcfg & IRQCRi_IRQMD_MASK), IRQCRi_REG(irqn));
}

int ra_icu_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			       void (*routine)(const void *parameter), const void *parameter,
			       uint32_t flags)
{
	uint32_t event = ((flags & RA_ICU_FLAG_EVENT_MASK) >> RA_ICU_FLAG_EVENT_OFFSET);
	uint32_t intcfg = ((flags & RA_ICU_FLAG_INTCFG_MASK) >> RA_ICU_FLAG_INTCFG_OFFSET);
	int irqn = irq;

	if (irq == RA_ICU_IRQ_UNSPECIFIED) {
		irqn = ra_icu_query_available_irq(event);
		if (irqn < 0) {
			return irqn;
		}
	}

	irq_disable(irqn);
	sys_write32(event, IELSRn_REG(irqn));
	z_isr_install(irqn, routine, parameter);
	z_arm_irq_priority_set(irqn, priority, flags);
	ra_icu_irq_configure(event, intcfg);

	return irqn;
}

int ra_icu_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
				  void (*routine)(const void *parameter), const void *parameter,
				  uint32_t flags)
{
	int irqn = irq;

	if (irq == RA_ICU_IRQ_UNSPECIFIED) {
		return -EINVAL;
	}

	irq_disable(irqn);
	sys_write32(0, IELSRn_REG(irqn));
	z_isr_install(irqn, z_irq_spurious, NULL);
	z_arm_irq_priority_set(irqn, 0, 0);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

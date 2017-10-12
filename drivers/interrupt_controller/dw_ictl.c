/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This implementation supports only the regular irqs
 * No support for priority filtering
 * No support for vectored interrupts
 * Firqs are also not supported
 * This implementation works only when sw_isr_table is enabled in zephyr
 */

#include <device.h>
#include <board.h>
#include <irq_nextlevel.h>
#include "dw_ictl.h"

static ALWAYS_INLINE void dw_ictl_dispatch_child_isrs(u32_t intr_status,
						      u32_t isr_base_offset)
{
	u32_t intr_bitpos, intr_offset;

	/* Dispatch lower level ISRs depending upon the bit set */
	while (intr_status) {
		intr_bitpos = find_lsb_set(intr_status) - 1;
		intr_status &= ~(1 << intr_bitpos);
		intr_offset = isr_base_offset + intr_bitpos;
		_sw_isr_table[intr_offset].isr(
			_sw_isr_table[intr_offset].arg);
	}
}

static int dw_ictl_initialize(struct device *port)
{
	struct dw_ictl_runtime *dw = port->driver_data;

	volatile struct dw_ictl_registers * const regs =
			(struct dw_ictl_registers *)dw->base_addr;

	/* disable all interrupts */
	regs->irq_inten_l = 0;
	regs->irq_inten_h = 0;

	return 0;
}

static void dw_ictl_isr(void *arg)
{
	struct device *port = (struct device *)arg;
	struct dw_ictl_runtime * const dw = port->driver_data;

	const struct dw_ictl_config *config = port->config->config_info;

	volatile struct dw_ictl_registers * const regs =
			(struct dw_ictl_registers *)dw->base_addr;

	dw_ictl_dispatch_child_isrs(regs->irq_maskstatus_l,
					config->isr_table_offset);

	if (config->numirqs > 32) {
		dw_ictl_dispatch_child_isrs(regs->irq_maskstatus_h,
						config->isr_table_offset + 32);
	}
}

static inline void dw_ictl_intr_enable(struct device *dev, unsigned int irq)
{
	struct dw_ictl_runtime *context = dev->driver_data;

	volatile struct dw_ictl_registers * const regs =
		(struct dw_ictl_registers *)context->base_addr;

	if (irq < 32) {
		regs->irq_inten_l |= (1 << irq);
	} else {
		regs->irq_inten_h |= (1 << (irq - 32));
	}
}

static inline void dw_ictl_intr_disable(struct device *dev, unsigned int irq)
{
	struct dw_ictl_runtime *context = dev->driver_data;

	volatile struct dw_ictl_registers * const regs =
		(struct dw_ictl_registers *)context->base_addr;

	if (irq < 32) {
		regs->irq_inten_l &= ~(1 << irq);
	} else {
		regs->irq_inten_h &= ~(1 << (irq - 32));
	}
}

static inline unsigned int dw_ictl_intr_get_state(struct device *dev)
{
	struct dw_ictl_runtime *context = dev->driver_data;

	const struct dw_ictl_config *config = dev->config->config_info;

	volatile struct dw_ictl_registers * const regs =
		(struct dw_ictl_registers *)context->base_addr;

	if (regs->irq_inten_l) {
		return 1;
	}

	if (config->numirqs > 32) {
		if (regs->irq_inten_h) {
			return 1;
		}
	}
	return 0;
}

static void dw_ictl_config_irq(struct device *port);

static const struct dw_ictl_config dw_config = {
	.irq_num = DW_ICTL_IRQ,
	.numirqs = DW_ICTL_NUM_IRQS,
	.isr_table_offset = CONFIG_DW_ISR_TBL_OFFSET,
	.config_func = dw_ictl_config_irq,
};

static struct dw_ictl_runtime dw_runtime = {
	.base_addr = DW_ICTL_BASE_ADDR,
};

static const struct irq_next_level_api dw_ictl_apis = {
	.intr_enable = dw_ictl_intr_enable,
	.intr_disable = dw_ictl_intr_disable,
	.intr_get_state = dw_ictl_intr_get_state,
};

DEVICE_AND_API_INIT(dw_ictl, CONFIG_DW_ICTL_NAME, dw_ictl_initialize,
		    &dw_runtime, &dw_config,
		    POST_KERNEL, CONFIG_DW_ICTL_INIT_PRIORITY, &dw_ictl_apis);

static void dw_ictl_config_irq(struct device *port)
{
	IRQ_CONNECT(DW_ICTL_IRQ, CONFIG_DW_ICTL_IRQ_PRI, dw_ictl_isr,
		    DEVICE_GET(dw_ictl), DW_ICTL_IRQ_FLAGS);
}

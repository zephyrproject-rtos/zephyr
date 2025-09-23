/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_intc

/* This implementation supports only the regular irqs
 * No support for priority filtering
 * No support for vectored interrupts
 * Firqs are also not supported
 * This implementation works only when sw_isr_table is enabled in zephyr
 */

#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/sw_isr_table.h>
#include "intc_dw.h"
#include <soc.h>
#include <zephyr/irq.h>

static ALWAYS_INLINE void dw_ictl_dispatch_child_isrs(uint32_t intr_status,
						      uint32_t isr_base_offset)
{
	uint32_t intr_bitpos, intr_offset;

	/* Dispatch lower level ISRs depending upon the bit set */
	while (intr_status) {
		intr_bitpos = find_lsb_set(intr_status) - 1;
		intr_status &= ~(1 << intr_bitpos);
		intr_offset = isr_base_offset + intr_bitpos - CONFIG_GEN_IRQ_START_VECTOR;
		_sw_isr_table[intr_offset].isr(
			_sw_isr_table[intr_offset].arg);
	}
}

static int dw_ictl_initialize(const struct device *dev)
{
	const struct dw_ictl_config *config = dev->config;
	volatile struct dw_ictl_registers * const regs =
			(struct dw_ictl_registers *)config->base_addr;

	/* disable all interrupts */
	regs->irq_inten_l = 0U;
	regs->irq_inten_h = 0U;

	config->config_func();

	return 0;
}

static void dw_ictl_isr(const struct device *dev)
{
	const struct dw_ictl_config *config = dev->config;
	volatile struct dw_ictl_registers * const regs =
			(struct dw_ictl_registers *)config->base_addr;

	dw_ictl_dispatch_child_isrs(regs->irq_finalstatus_l,
				    config->isr_table_offset);

	if (config->numirqs > 32) {
		dw_ictl_dispatch_child_isrs(regs->irq_finalstatus_h,
					    config->isr_table_offset + 32);
	}
}

static inline void dw_ictl_intr_enable(const struct device *dev,
				       unsigned int irq)
{
	const struct dw_ictl_config *config = dev->config;
	volatile struct dw_ictl_registers * const regs =
		(struct dw_ictl_registers *)config->base_addr;

	if (irq < 32) {
		regs->irq_inten_l |= (1 << irq);
	} else {
		regs->irq_inten_h |= (1 << (irq - 32));
	}
}

static inline void dw_ictl_intr_disable(const struct device *dev,
					unsigned int irq)
{
	const struct dw_ictl_config *config = dev->config;
	volatile struct dw_ictl_registers * const regs =
		(struct dw_ictl_registers *)config->base_addr;

	if (irq < 32) {
		regs->irq_inten_l &= ~(1 << irq);
	} else {
		regs->irq_inten_h &= ~(1 << (irq - 32));
	}
}

static inline unsigned int dw_ictl_intr_get_state(const struct device *dev)
{
	const struct dw_ictl_config *config = dev->config;
	volatile struct dw_ictl_registers * const regs =
		(struct dw_ictl_registers *)config->base_addr;

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

static int dw_ictl_intr_get_line_state(const struct device *dev,
				       unsigned int irq)
{
	const struct dw_ictl_config *config = dev->config;
	volatile struct dw_ictl_registers * const regs =
		(struct dw_ictl_registers *)config->base_addr;

	if (config->numirqs > 32) {
		if ((regs->irq_inten_h & BIT(irq - 32)) != 0) {
			return 1;
		}
	} else {
		if ((regs->irq_inten_l & BIT(irq)) != 0) {
			return 1;
		}
	}

	return 0;
}

static const struct irq_next_level_api dw_ictl_apis = {
	.intr_enable = dw_ictl_intr_enable,
	.intr_disable = dw_ictl_intr_disable,
	.intr_get_state = dw_ictl_intr_get_state,
	.intr_get_line_state = dw_ictl_intr_get_line_state,
};

#define INTC_DW_DEVICE_INIT(inst)                                                                  \
                                                                                                   \
	static void dw_ictl_config_irq_##inst(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), dw_ictl_isr,          \
			    DEVICE_DT_INST_GET(inst), DT_INST_IRQ(inst, sense));                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
	IRQ_PARENT_ENTRY_DEFINE(intc_dw##inst, DEVICE_DT_INST_GET(inst), DT_INST_IRQN(inst),       \
				INTC_INST_ISR_TBL_OFFSET(inst),                                    \
				DT_INST_INTC_GET_AGGREGATOR_LEVEL(inst));                          \
                                                                                                   \
	static const struct dw_ictl_config dw_config_##inst = {                                    \
		.base_addr = DT_INST_REG_ADDR(inst),                                               \
		.numirqs = DT_INST_PROP(inst, num_irqs),                                           \
		.isr_table_offset = INTC_INST_ISR_TBL_OFFSET(inst),                                \
		.config_func = dw_ictl_config_irq_##inst,                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dw_ictl_initialize, NULL, NULL, &dw_config_##inst,             \
			      PRE_KERNEL_1, CONFIG_DW_ICTL_INIT_PRIORITY, &dw_ictl_apis);

DT_INST_FOREACH_STATUS_OKAY(INTC_DW_DEVICE_INIT)

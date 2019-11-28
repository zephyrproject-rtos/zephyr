/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <irq_nextlevel.h>
#include "cavs_ictl.h"

static ALWAYS_INLINE void cavs_ictl_dispatch_child_isrs(u32_t intr_status,
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

static void cavs_ictl_isr(void *arg)
{
	struct device *port = (struct device *)arg;
	struct cavs_ictl_runtime *context = port->driver_data;

	const struct cavs_ictl_config *config = port->config->config_info;

	volatile struct cavs_registers * const regs =
			(struct cavs_registers *)context->base_addr;

	cavs_ictl_dispatch_child_isrs(regs->status_il,
				      config->isr_table_offset);
}

static inline void cavs_ictl_irq_enable(struct device *dev, unsigned int irq)
{
	struct cavs_ictl_runtime *context = dev->driver_data;

	volatile struct cavs_registers * const regs =
			(struct cavs_registers *)context->base_addr;

	regs->enable_il = (1 << irq);
}

static inline void cavs_ictl_irq_disable(struct device *dev, unsigned int irq)
{
	struct cavs_ictl_runtime *context = dev->driver_data;

	volatile struct cavs_registers * const regs =
			(struct cavs_registers *)context->base_addr;

	regs->disable_il = (1 << irq);
}

static inline unsigned int cavs_ictl_irq_get_state(struct device *dev)
{
	struct cavs_ictl_runtime *context = dev->driver_data;

	volatile struct cavs_registers * const regs =
			(struct cavs_registers *)context->base_addr;

	/* When the bits of this register are set, it means the
	 * corresponding interrupts are disabled. This function
	 * returns 0 only if ALL the interrupts are disabled.
	 */
	if (regs->disable_state_il == 0xFFFFFFFF) {
		return 0;
	}

	return 1;
}

static int cavs_ictl_irq_get_line_state(struct device *dev, unsigned int irq)
{
	struct cavs_ictl_runtime *context = dev->driver_data;

	volatile struct cavs_registers * const regs =
			(struct cavs_registers *)context->base_addr;

	if ((regs->disable_state_il & BIT(irq)) == 0) {
		return 1;
	}

	return 0;
}

static const struct irq_next_level_api cavs_apis = {
	.intr_enable = cavs_ictl_irq_enable,
	.intr_disable = cavs_ictl_irq_disable,
	.intr_get_state = cavs_ictl_irq_get_state,
	.intr_get_line_state = cavs_ictl_irq_get_line_state,
};

static int cavs_ictl_0_initialize(struct device *port)
{
	return 0;
}

static void cavs_config_0_irq(struct device *port);

static const struct cavs_ictl_config cavs_config_0 = {
	.irq_num = DT_CAVS_ICTL_0_IRQ,
	.isr_table_offset = CONFIG_CAVS_ISR_TBL_OFFSET,
	.config_func = cavs_config_0_irq,
};

static struct cavs_ictl_runtime cavs_0_runtime = {
	.base_addr = DT_CAVS_ICTL_BASE_ADDR,
};

DEVICE_AND_API_INIT(cavs_ictl_0, CONFIG_CAVS_ICTL_0_NAME,
		    cavs_ictl_0_initialize, &cavs_0_runtime, &cavs_config_0,
		    POST_KERNEL, CONFIG_CAVS_ICTL_INIT_PRIORITY, &cavs_apis);

static void cavs_config_0_irq(struct device *port)
{
	IRQ_CONNECT(DT_CAVS_ICTL_0_IRQ, DT_CAVS_ICTL_0_IRQ_PRI, cavs_ictl_isr,
		    DEVICE_GET(cavs_ictl_0), DT_CAVS_ICTL_0_IRQ_FLAGS);
}

static int cavs_ictl_1_initialize(struct device *port)
{
	return 0;
}

static void cavs_config_1_irq(struct device *port);

static const struct cavs_ictl_config cavs_config_1 = {
	.irq_num = DT_CAVS_ICTL_1_IRQ,
	.isr_table_offset = CONFIG_CAVS_ISR_TBL_OFFSET +
			    CONFIG_MAX_IRQ_PER_AGGREGATOR,
	.config_func = cavs_config_1_irq,
};

static struct cavs_ictl_runtime cavs_1_runtime = {
	.base_addr = DT_CAVS_ICTL_BASE_ADDR + sizeof(struct cavs_registers),
};

DEVICE_AND_API_INIT(cavs_ictl_1, CONFIG_CAVS_ICTL_1_NAME,
		    cavs_ictl_1_initialize, &cavs_1_runtime, &cavs_config_1,
		    POST_KERNEL, CONFIG_CAVS_ICTL_INIT_PRIORITY, &cavs_apis);

static void cavs_config_1_irq(struct device *port)
{
	IRQ_CONNECT(DT_CAVS_ICTL_1_IRQ, DT_CAVS_ICTL_1_IRQ_PRI, cavs_ictl_isr,
		    DEVICE_GET(cavs_ictl_1), DT_CAVS_ICTL_1_IRQ_FLAGS);
}

static int cavs_ictl_2_initialize(struct device *port)
{
	return 0;
}

static void cavs_config_2_irq(struct device *port);

static const struct cavs_ictl_config cavs_config_2 = {
	.irq_num = DT_CAVS_ICTL_2_IRQ,
	.isr_table_offset = CONFIG_CAVS_ISR_TBL_OFFSET +
			    CONFIG_MAX_IRQ_PER_AGGREGATOR * 2,
	.config_func = cavs_config_2_irq,
};

static struct cavs_ictl_runtime cavs_2_runtime = {
	.base_addr = DT_CAVS_ICTL_BASE_ADDR + sizeof(struct cavs_registers) * 2,
};

DEVICE_AND_API_INIT(cavs_ictl_2, CONFIG_CAVS_ICTL_2_NAME,
		    cavs_ictl_2_initialize, &cavs_2_runtime, &cavs_config_2,
		    POST_KERNEL, CONFIG_CAVS_ICTL_INIT_PRIORITY, &cavs_apis);

static void cavs_config_2_irq(struct device *port)
{
	IRQ_CONNECT(DT_CAVS_ICTL_2_IRQ, DT_CAVS_ICTL_2_IRQ_PRI, cavs_ictl_isr,
		    DEVICE_GET(cavs_ictl_2), DT_CAVS_ICTL_2_IRQ_FLAGS);
}

static int cavs_ictl_3_initialize(struct device *port)
{
	return 0;
}

static void cavs_config_3_irq(struct device *port);

static const struct cavs_ictl_config cavs_config_3 = {
	.irq_num = DT_CAVS_ICTL_3_IRQ,
	.isr_table_offset = CONFIG_CAVS_ISR_TBL_OFFSET +
			    CONFIG_MAX_IRQ_PER_AGGREGATOR*3,
	.config_func = cavs_config_3_irq,
};

static struct cavs_ictl_runtime cavs_3_runtime = {
	.base_addr = DT_CAVS_ICTL_BASE_ADDR + sizeof(struct cavs_registers) * 3,
};

DEVICE_AND_API_INIT(cavs_ictl_3, CONFIG_CAVS_ICTL_3_NAME,
		    cavs_ictl_3_initialize, &cavs_3_runtime, &cavs_config_3,
		    POST_KERNEL, CONFIG_CAVS_ICTL_INIT_PRIORITY, &cavs_apis);

static void cavs_config_3_irq(struct device *port)
{
	IRQ_CONNECT(DT_CAVS_ICTL_3_IRQ, DT_CAVS_ICTL_3_IRQ_PRI, cavs_ictl_isr,
		    DEVICE_GET(cavs_ictl_3), DT_CAVS_ICTL_3_IRQ_FLAGS);
}

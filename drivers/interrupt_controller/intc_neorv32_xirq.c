/*
 * Copyright (c) 2024 Calian Advanced Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT neorv32_xirq

#include <zephyr/device.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq_multilevel.h>
#include <soc.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(intc_neorv32_xirq, CONFIG_LOG_DEFAULT_LEVEL);

/* NEORV32 XIRQ registers offsets */
#define NEORV32_XIRQ_INT_ENABLE_OFFSET	0x00
#define NEORV32_XIRQ_INT_PENDING_OFFSET 0x04
#define NEORV32_XIRQ_INT_SOURCE_OFFSET	0x08

struct neorv32_xirq_config {
	mm_reg_t base_addr;
};

struct neorv32_xirq_data {
	struct k_spinlock lock;
};

static void neorv32_xirq_isr(const struct device *dev)
{
	const struct neorv32_xirq_config *config = dev->config;
	const uint32_t trigger_type = DT_INST_PROP(0, trigger_type);
	bool is_edge_irq;
	uint32_t intr_offset;
	uint32_t src;

	src = sys_read32(config->base_addr + NEORV32_XIRQ_INT_SOURCE_OFFSET);
	is_edge_irq = (trigger_type & BIT(src)) != 0;

	/**
	 * If the interrupt is edge-triggered, clear the interrupt before running the ISR
	 * so that newly raised interrupts will trigger another XIRQ interrupt.
	 * If the interrupt is level-triggered, clear the interrupt after running the ISR
	 * so that the ISR has had a chance to clear the condition that caused the interrupt.
	 */
	if (is_edge_irq) {
		sys_write32(~BIT(src), config->base_addr + NEORV32_XIRQ_INT_PENDING_OFFSET);
		sys_write32(0, config->base_addr + NEORV32_XIRQ_INT_SOURCE_OFFSET);
	}

	intr_offset = CONFIG_2ND_LVL_ISR_TBL_OFFSET + src;
	_sw_isr_table[intr_offset].isr(_sw_isr_table[intr_offset].arg);

	if (!is_edge_irq) {
		sys_write32(~BIT(src), config->base_addr + NEORV32_XIRQ_INT_PENDING_OFFSET);
		sys_write32(0, config->base_addr + NEORV32_XIRQ_INT_SOURCE_OFFSET);
	}
}

static void neorv32_xirq_intr_enable(const struct device *dev,
				       unsigned int irq)
{
	const struct neorv32_xirq_config *config = dev->config;
	struct neorv32_xirq_data *data = dev->data;
	uint32_t enable_mask;
	const unsigned int local_irq = irq_from_level_2(irq);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	enable_mask = sys_read32(config->base_addr + NEORV32_XIRQ_INT_ENABLE_OFFSET);
	enable_mask |= BIT(local_irq);
	sys_write32(~BIT(local_irq), config->base_addr + NEORV32_XIRQ_INT_PENDING_OFFSET);
	sys_write32(enable_mask, config->base_addr + NEORV32_XIRQ_INT_ENABLE_OFFSET);
	k_spin_unlock(&data->lock, key);
}

static void neorv32_xirq_intr_disable(const struct device *dev,
					unsigned int irq)
{
	const struct neorv32_xirq_config *config = dev->config;
	struct neorv32_xirq_data *data = dev->data;
	uint32_t enable_mask;
	const unsigned int local_irq = irq_from_level_2(irq);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	enable_mask = sys_read32(config->base_addr + NEORV32_XIRQ_INT_ENABLE_OFFSET);
	enable_mask &= ~BIT(local_irq);
	sys_write32(enable_mask, config->base_addr + NEORV32_XIRQ_INT_ENABLE_OFFSET);
	k_spin_unlock(&data->lock, key);
}

static unsigned int neorv32_xirq_intr_get_state(const struct device *dev)
{
	const struct neorv32_xirq_config *config = dev->config;

	return sys_read32(config->base_addr + NEORV32_XIRQ_INT_ENABLE_OFFSET) != 0;
}

static int neorv32_xirq_intr_get_line_state(const struct device *dev,
				       unsigned int irq)
{
	const struct neorv32_xirq_config *config = dev->config;
	const unsigned int local_irq = irq_from_level_2(irq);

	return (sys_read32(config->base_addr + NEORV32_XIRQ_INT_ENABLE_OFFSET) &
		BIT(local_irq)) != 0;
}

static const struct neorv32_xirq_config neorv32_xirq_config_inst = {
	.base_addr = DT_INST_REG_ADDR(0),
};

static struct neorv32_xirq_data neorv32_xirq_data_inst;

static const struct irq_next_level_api neorv32_xirq_apis = {
	.intr_enable = neorv32_xirq_intr_enable,
	.intr_disable = neorv32_xirq_intr_disable,
	.intr_get_state = neorv32_xirq_intr_get_state,
	.intr_get_line_state = neorv32_xirq_intr_get_line_state,
};

static int neorv32_xirq_initialize(const struct device *dev)
{
	const struct neorv32_xirq_config *config = dev->config;
	const struct device *syscon = DEVICE_DT_GET(DT_INST_PHANDLE(0, syscon));
	uint32_t features;
	int err;

	err = syscon_read_reg(syscon, NEORV32_SYSINFO_FEATURES, &features);
	if (err < 0) {
		LOG_ERR("failed to determine implemented features (err %d)", err);
		return -EIO;
	}

	if ((features & NEORV32_SYSINFO_FEATURES_IO_XIRQ) == 0) {
		LOG_ERR("NEORV32 XIRQ not supported");
		return -ENODEV;
	}

	/* disable and clear all interrupts, clear pending */
	sys_write32(0, config->base_addr + NEORV32_XIRQ_INT_ENABLE_OFFSET);
	sys_write32(0, config->base_addr + NEORV32_XIRQ_INT_PENDING_OFFSET);
	sys_write32(0, config->base_addr + NEORV32_XIRQ_INT_SOURCE_OFFSET);

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    neorv32_xirq_isr,
		    DEVICE_DT_INST_GET(0),
		    DT_INST_IRQ(0, sense));
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

DEVICE_DT_INST_DEFINE(0, neorv32_xirq_initialize, NULL,
		&neorv32_xirq_data_inst, &neorv32_xirq_config_inst, PRE_KERNEL_1,
		CONFIG_NEORV32_XIRQ_INIT_PRIORITY, &neorv32_xirq_apis);

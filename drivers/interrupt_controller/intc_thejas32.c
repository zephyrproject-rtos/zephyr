/*
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cdac_thejas32_intc

/**
 * @brief C-DAC THEJAS32 interrupt controller driver
 *
 * The THEJAS32 interrupt controller is a simple second-level aggregator that
 * ORs the SoC peripheral interrupt sources into the RISC-V machine external
 * interrupt line. It exposes only an enable bitmask and an (enabled and
 * pending) status bitmask; there is no per-source priority and no
 * claim/complete protocol. A source is cleared by servicing the peripheral
 * that raised it, so the controller handler simply dispatches the registered
 * ISR for every pending source.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/interrupt_controller/intc_thejas32.h>

#include "sw_isr_common.h"

/* Register offsets from the controller base address. */
#define THEJAS32_INTC_RAW_INTR    0x00
#define THEJAS32_INTC_INTR_EN     0x08
#define THEJAS32_INTC_INTR_STATUS 0x10

struct thejas32_intc_config {
	mem_addr_t base;
	uint32_t nr_irqs;
	uint32_t irq;
	const struct _isr_table_entry *isr_table;
	void (*irq_config_func)(void);
};

static const struct device *const thejas32_intc_dev = DEVICE_DT_INST_GET(0);

static inline mem_addr_t thejas32_intc_en_reg(const struct device *dev)
{
	const struct thejas32_intc_config *config = dev->config;

	return config->base + THEJAS32_INTC_INTR_EN;
}

void thejas32_intc_irq_enable(uint32_t irq)
{
	const struct device *dev = thejas32_intc_dev;
	const uint32_t local_irq = irq_from_level_2(irq);
	const mem_addr_t en = thejas32_intc_en_reg(dev);
	unsigned int key;

	key = irq_lock();
	sys_write32(sys_read32(en) | BIT(local_irq), en);
	irq_unlock(key);
}

void thejas32_intc_irq_disable(uint32_t irq)
{
	const struct device *dev = thejas32_intc_dev;
	const uint32_t local_irq = irq_from_level_2(irq);
	const mem_addr_t en = thejas32_intc_en_reg(dev);
	unsigned int key;

	key = irq_lock();
	sys_write32(sys_read32(en) & ~BIT(local_irq), en);
	irq_unlock(key);
}

int thejas32_intc_irq_is_enabled(uint32_t irq)
{
	const struct device *dev = thejas32_intc_dev;
	const uint32_t local_irq = irq_from_level_2(irq);

	return (sys_read32(thejas32_intc_en_reg(dev)) & BIT(local_irq)) != 0;
}

static void thejas32_intc_irq_handler(const struct device *dev)
{
	const struct thejas32_intc_config *config = dev->config;
	uint32_t status = sys_read32(config->base + THEJAS32_INTC_INTR_STATUS);

	while (status != 0U) {
		const uint32_t local_irq = find_lsb_set(status) - 1;
		const struct _isr_table_entry *ite = &config->isr_table[local_irq];

		status &= ~BIT(local_irq);

		if (local_irq >= config->nr_irqs) {
			z_irq_spurious(NULL);
		}

		ite->isr(ite->arg);
	}
}

static int thejas32_intc_init(const struct device *dev)
{
	const struct thejas32_intc_config *config = dev->config;

	/* Mask all sources until drivers enable their own. */
	sys_write32(0U, config->base + THEJAS32_INTC_INTR_EN);

	config->irq_config_func();

	return 0;
}

#define THEJAS32_INTC_NR_IRQS(n) CONFIG_MAX_IRQ_PER_AGGREGATOR

#define THEJAS32_INTC_INIT(n)                                                                      \
	static void thejas32_intc_irq_config_func_##n(void)                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, thejas32_intc_irq_handler, DEVICE_DT_INST_GET(n),  \
			    0);                                                                    \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static const struct thejas32_intc_config thejas32_intc_config_##n = {                      \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.nr_irqs = THEJAS32_INTC_NR_IRQS(n),                                               \
		.irq = DT_INST_IRQN(n),                                                            \
		.isr_table = &_sw_isr_table[INTC_INST_ISR_TBL_OFFSET(n)],                          \
		.irq_config_func = thejas32_intc_irq_config_func_##n,                              \
	};                                                                                         \
	IRQ_PARENT_ENTRY_DEFINE(thejas32_intc_##n, DEVICE_DT_INST_GET(n), DT_INST_IRQN(n),         \
				INTC_INST_ISR_TBL_OFFSET(n),                                       \
				DT_INST_INTC_GET_AGGREGATOR_LEVEL(n));                             \
	DEVICE_DT_INST_DEFINE(n, &thejas32_intc_init, NULL, NULL, &thejas32_intc_config_##n,       \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(THEJAS32_INTC_INIT)

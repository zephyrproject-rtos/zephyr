/*
 * Copyright (c) 2023, Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_syscfg

/**
 * @brief Driver for System Configuration Controller (SYSCFG) in STM32 MCUs
 */

#include <zephyr/device.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <soc.h>

/* By design this can only support 32 second level interrupts, more than enough for STM32 MCUs. */
BUILD_ASSERT((CONFIG_MAX_IRQ_PER_AGGREGATOR > 0) && (CONFIG_MAX_IRQ_PER_AGGREGATOR <= 32),
	     "unsupported number of interrupts");

/* CONFIG_2ND_LVL_ISR_TBL_OFFSET is number of 1st level interrupts */
#define NUM_1ST_LVL_IRQS CONFIG_2ND_LVL_ISR_TBL_OFFSET

struct syscfg_config {
	SYSCFG_TypeDef *base;
	struct stm32_pclken pclken;
};

struct syscfg_data {
	/* List of IT lines child devices */
	const struct device *itline_devs[NUM_1ST_LVL_IRQS];
};

struct syscfg_itline_config {
	unsigned int irq_lvl1;
	void (*irq_cfg_func)(void);
};

struct syscfg_itline_data {
	unsigned int isr_table_offset;
	/* 2nd level IRQ enabled bitmask */
	uint32_t irq_enabled;
};

/*
 * Mapping between the interrupt number and the offset in the _isr_table_entry
 */
struct irq_parent_offset {
	unsigned int irq;
	unsigned int offset;
};

#define INIT_IRQ_PARENT_OFFSET(i, o)                                                               \
	{                                                                                          \
		.irq = i,                                                                          \
		.offset = o,                                                                       \
	}

#define IRQ_INDEX_TO_OFFSET(i, base) (base + i * CONFIG_MAX_IRQ_PER_AGGREGATOR)

#define CAT_2ND_LVL_LIST(i, base)                                                                  \
	INIT_IRQ_PARENT_OFFSET(CONFIG_2ND_LVL_INTR_0##i##_OFFSET, IRQ_INDEX_TO_OFFSET(i, base))

static const struct irq_parent_offset lvl2_irq_list[CONFIG_NUM_2ND_LEVEL_AGGREGATORS] = {LISTIFY(
	CONFIG_NUM_2ND_LEVEL_AGGREGATORS, CAT_2ND_LVL_LIST, (,), CONFIG_2ND_LVL_ISR_TBL_OFFSET) };

/*
 * <irq_nextlevel.h> API
 */

static void syscfg_intr_enable(const struct device *dev, uint32_t irq)
{
	uint8_t irq_lvl1 = irq_parent_level_2(irq);
	uint8_t irq_lvl2 = irq_from_level_2(irq);
	const struct device *itline_dev = NULL;
	struct syscfg_data *data = dev->data;
	const struct syscfg_itline_config *itline_config = NULL;
	struct syscfg_itline_data *itline_data = NULL;

	if (irq_lvl1 >= NUM_1ST_LVL_IRQS) {
		return;
	}

	if (irq_lvl2 >= CONFIG_MAX_IRQ_PER_AGGREGATOR) {
		return;
	}

	itline_dev = data->itline_devs[irq_lvl1];
	if (!itline_dev) {
		return;
	}

	itline_config = itline_dev->config;
	itline_data = itline_dev->data;

	if (itline_config->irq_lvl1 != irq_lvl1) {
		return;
	}

	itline_data->irq_enabled |= BIT(irq_lvl2);
	irq_enable(irq_lvl1);
}

static void syscfg_intr_disable(const struct device *dev, uint32_t irq)
{
	uint8_t irq_lvl1 = irq_parent_level_2(irq);
	uint8_t irq_lvl2 = irq_from_level_2(irq);
	const struct device *itline_dev = NULL;
	struct syscfg_data *data = dev->data;
	const struct syscfg_itline_config *itline_config = NULL;
	struct syscfg_itline_data *itline_data = NULL;

	if (irq_lvl1 >= NUM_1ST_LVL_IRQS) {
		return;
	}

	if (irq_lvl2 >= CONFIG_MAX_IRQ_PER_AGGREGATOR) {
		return;
	}

	itline_dev = data->itline_devs[irq_lvl1];
	if (!itline_dev) {
		return;
	}

	itline_config = itline_dev->config;
	itline_data = itline_dev->data;

	if (itline_config->irq_lvl1 != irq_lvl1) {
		return;
	}

	itline_data->irq_enabled &= ~BIT(irq_lvl2);

	/* Disable the 1st level interrupt if all the second ones are disabled. */
	if (itline_data->irq_enabled == 0U) {
		irq_disable(irq_lvl1);
	}
}

static unsigned int syscfg_intr_get_state(const struct device *dev)
{
	const struct device *itline_dev = NULL;
	struct syscfg_data *data = dev->data;
	struct syscfg_itline_data *itline_data = NULL;

	for (uint8_t irq_lvl1 = 0U; irq_lvl1 < NUM_1ST_LVL_IRQS; irq_lvl1++) {
		itline_dev = data->itline_devs[irq_lvl1];
		if (!itline_dev) {
			continue;
		}

		itline_data = itline_dev->data;
		if (itline_data->irq_enabled) {
			return 1;
		}
	}

	return 0;
}

static int syscfg_intr_get_line_state(const struct device *dev, unsigned int irq)
{
	const struct device *itline_dev = NULL;
	struct syscfg_data *data = dev->data;
	struct syscfg_itline_data *itline_data = NULL;

	if (irq >= NUM_1ST_LVL_IRQS) {
		return 0;
	}

	itline_dev = data->itline_devs[irq];
	if (!itline_dev) {
		return 0;
	}

	itline_data = itline_dev->data;

	return (itline_data->irq_enabled != 0U);
}

/*
 * IRQ handling.
 */

static void syscfg_itline_isr(const struct device *dev)
{
	const struct syscfg_itline_config *config = dev->config;
	struct syscfg_itline_data *data = dev->data;
	uint32_t sr = SYSCFG->IT_LINE_SR[config->irq_lvl1] & data->irq_enabled;

	/* Dispatch lower level ISRs depending upon the bit set */
	while (sr) {
		int bit = find_lsb_set(sr) - 1;
		struct _isr_table_entry *ent = NULL;

		ent = &_sw_isr_table[data->isr_table_offset + bit];
		if (ent && ent->isr) {
			ent->isr(ent->arg);
		}

		sr &= ~BIT(bit);
	}
}

/*
 * Instance and initialization
 */

static const struct irq_next_level_api syscfg_apis = {
	.intr_enable = syscfg_intr_enable,
	.intr_disable = syscfg_intr_disable,
	.intr_get_state = syscfg_intr_get_state,
	.intr_get_line_state = syscfg_intr_get_line_state,
};

static struct syscfg_data syscfg_data = {
	.itline_devs = {NULL},
};

static const struct syscfg_config syscfg_config = {
	.base = (SYSCFG_TypeDef *)DT_REG_ADDR(DT_DRV_INST(0)),
	.pclken = STM32_CLOCK_INFO(0, DT_DRV_INST(0)),
};

static void syscfg_register_itline(const struct device *dev, uint8_t irq,
				   const struct device *itline_dev)
{
	struct syscfg_data *data = dev->data;

	if (irq >= NUM_1ST_LVL_IRQS) {
		return;
	}

	data->itline_devs[irq] = itline_dev;
}

static int syscfg_itline_init(const struct device *dev)
{
	const struct syscfg_itline_config *config = dev->config;
	struct syscfg_itline_data *data = dev->data;
	unsigned int i;

	/* Find the offset in the _isr_table_entry for that parent interrupt */
	for (i = 0U; i < CONFIG_NUM_2ND_LEVEL_AGGREGATORS; i++) {
		if (lvl2_irq_list[i].irq == config->irq_lvl1) {
			data->isr_table_offset = lvl2_irq_list[i].offset;
			break;
		}
	}
	if (i == CONFIG_NUM_2ND_LEVEL_AGGREGATORS) {
		return -EINVAL;
	}

	config->irq_cfg_func();

	return 0;
}

static int syscfg_init(const struct device *dev)
{
	const struct syscfg_config *config = dev->config;

	/* enable clock for SYSCFG device */
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&config->pclken) != 0) {
		return -EIO;
	}

	return 0;
}

DEVICE_DT_INST_DEFINE(0, &syscfg_init, NULL, &syscfg_data, &syscfg_config, PRE_KERNEL_1,
		      CONFIG_INTC_INIT_PRIORITY, &syscfg_apis);

#define SYSCFG_ITLINE_INIT(node_id)                                                                \
                                                                                                   \
	static void syscfg_itline_irq_config_func_##node_id(void)                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(node_id), DT_IRQ(node_id, priority), syscfg_itline_isr,        \
			    DEVICE_DT_GET(node_id), 0);                                            \
		syscfg_register_itline(DEVICE_DT_GET(DT_PARENT(node_id)), DT_IRQN(node_id),        \
				       DEVICE_DT_GET(node_id));                                    \
	}                                                                                          \
                                                                                                   \
	const struct syscfg_itline_config syscfg_itline_config_##node_id = {                       \
		.irq_lvl1 = DT_IRQN(node_id),                                                      \
		.irq_cfg_func = syscfg_itline_irq_config_func_##node_id,                           \
	};                                                                                         \
                                                                                                   \
	static struct syscfg_itline_data syscfg_itline_data_##node_id = {                          \
		.isr_table_offset = 0,                                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, syscfg_itline_init, NULL, &syscfg_itline_data_##node_id,         \
			 &syscfg_itline_config_##node_id, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, \
			 NULL);

DT_INST_FOREACH_CHILD_STATUS_OKAY(0, SYSCFG_ITLINE_INIT);

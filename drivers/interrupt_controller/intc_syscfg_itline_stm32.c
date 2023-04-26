/*
 * Copyright (c) 2023, Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_syscfg_itline

/**
 * @brief Driver for System Configuration Controller (SYSCFG) Interrupt Line in STM32 MCUs
 */

#include <zephyr/device.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <soc.h>

/* By design this can only support 32 second level interrupts, more than enough for STM32 MCUs. */
BUILD_ASSERT((CONFIG_MAX_IRQ_PER_AGGREGATOR > 0) && (CONFIG_MAX_IRQ_PER_AGGREGATOR <= 32),
	     "unsupported number of interrupts");

struct syscfg_itline_config {
	SYSCFG_TypeDef *base;
	int reg;
	void (*irq_cfg_func)(void);
	unsigned int parent_irq;
	struct stm32_pclken pclken;
};

struct syscfg_itline_data {
	uint32_t irq_enabled;
	unsigned int isr_table_offset;
};

/*
 * Mapping between the interrupt number and the offset in the _isr_table_entry
 */
struct irq_parent_offset {
	unsigned int irq;
	unsigned int offset;
};

#define INIT_IRQ_PARENT_OFFSET(i, o) { \
	.irq = i, \
	.offset = o, \
}

#define IRQ_INDEX_TO_OFFSET(i, base) (base + i * CONFIG_MAX_IRQ_PER_AGGREGATOR)

#define CAT_2ND_LVL_LIST(i, base) \
	INIT_IRQ_PARENT_OFFSET(CONFIG_2ND_LVL_INTR_0##i##_OFFSET, \
		IRQ_INDEX_TO_OFFSET(i, base))
static const struct irq_parent_offset lvl2_irq_list[CONFIG_NUM_2ND_LEVEL_AGGREGATORS]
	= { LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, CAT_2ND_LVL_LIST, (,),
		CONFIG_2ND_LVL_ISR_TBL_OFFSET) };

/*
 * <irq_nextlevel.h> API
 */

static void syscfg_itline_enable(const struct device *dev, uint32_t irq)
{
	const struct syscfg_itline_config *config = dev->config;
	struct syscfg_itline_data *data = dev->data;

	if (irq >= CONFIG_MAX_IRQ_PER_AGGREGATOR) {
		return;
	}

	data->irq_enabled |= BIT(irq);
	irq_enable(config->parent_irq);
}

static void syscfg_itline_disable(const struct device *dev, uint32_t irq)
{
	const struct syscfg_itline_config *config = dev->config;
	struct syscfg_itline_data *data = dev->data;

	if (irq >= CONFIG_MAX_IRQ_PER_AGGREGATOR) {
		return;
	}

	data->irq_enabled &= ~BIT(irq);

	/* Disable the 1st level interrupt if all the second ones are disabled. */
	if (data->irq_enabled == 0) {
		irq_disable(config->parent_irq);
	}
}

static uint32_t syscfg_itline_get_state(const struct device *dev)
{
	const struct syscfg_itline_config *config = dev->config;
	struct syscfg_itline_data *data = dev->data;

	return (SYSCFG->IT_LINE_SR[config->reg] & data->irq_enabled) != 0;
}

static int syscfg_itline_get_line_state(const struct device *dev, unsigned int irq)
{
	const struct syscfg_itline_config *config = dev->config;
	struct syscfg_itline_data *data = dev->data;

	if (irq >= CONFIG_MAX_IRQ_PER_AGGREGATOR) {
		return 0;
	}

	return (SYSCFG->IT_LINE_SR[config->reg] & data->irq_enabled & BIT(irq)) != 0;
}

/*
 * IRQ handling.
 */

static void syscfg_itline_isr(const struct device *dev)
{
	const struct syscfg_itline_config *config = dev->config;
	struct syscfg_itline_data *data = dev->data;

	uint32_t sr = SYSCFG->IT_LINE_SR[config->reg] & data->irq_enabled;

	/* Dispatch lower level ISRs depending upon the bit set */
	while (sr) {
		int bit = find_lsb_set(sr) - 1;
		struct _isr_table_entry *ent = &_sw_isr_table[data->isr_table_offset + bit];

		sr &= ~BIT(bit);
		ent->isr(ent->arg);
	}
}

/*
 * Instance and initialization
 */

static const struct irq_next_level_api syscfg_itline_apis = {
	.intr_enable = syscfg_itline_enable,
	.intr_disable = syscfg_itline_disable,
	.intr_get_state = syscfg_itline_get_state,
	.intr_get_line_state = syscfg_itline_get_line_state,
};

static int syscfg_itline_init(const struct device *dev)
{
	const struct syscfg_itline_config *config = dev->config;
	struct syscfg_itline_data *data = dev->data;
	unsigned int i;

	/* enable clock for SYSCFG device */
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t) &config->pclken) != 0) {
		return -EIO;
	}

	/* Find the offset in the _isr_table_entry for that parent interrupt */
	for (i = 0U; i < CONFIG_NUM_2ND_LEVEL_AGGREGATORS; i++) {
		if (lvl2_irq_list[i].irq == config->parent_irq) {
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

#define SYSCFG_ITLINE_INIT(index)							\
											\
	static void syscfg_itline_irq_config_func_##index(void)				\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(index),					\
			    DT_INST_IRQ(index, priority),				\
			    syscfg_itline_isr, DEVICE_DT_INST_GET(index), 0);		\
	}										\
											\
	const struct syscfg_itline_config syscfg_itline_config_##index = {		\
		.base = (SYSCFG_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(index)),		\
		.reg = DT_INST_REG_ADDR(index),						\
		.irq_cfg_func = syscfg_itline_irq_config_func_##index,			\
		.parent_irq = DT_INST_IRQN(index),					\
		.pclken = STM32_CLOCK_INFO(0, DT_INST_PARENT(index)),			\
	};										\
											\
	static struct syscfg_itline_data syscfg_itline_data_##index = {			\
		.irq_enabled = 0,							\
		.isr_table_offset = 0,							\
	};										\
											\
	DEVICE_DT_INST_DEFINE(index, syscfg_itline_init, NULL,				\
			&syscfg_itline_data_##index, &syscfg_itline_config_##index,	\
			PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,			\
			&syscfg_itline_apis);


DT_INST_FOREACH_STATUS_OKAY(SYSCFG_ITLINE_INIT)

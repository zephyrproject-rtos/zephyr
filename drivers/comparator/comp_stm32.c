/*
 * Copyright (c) 2023 Adam Mitchell, Brill Power Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_comp

#include <errno.h>
#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_comp.h>

#define LOG_LEVEL CONFIG_COMPARATOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(comp_stm32);

struct comp_cfg {
	COMP_TypeDef *base;
	struct stm32_pclken pclken;
	const struct pinctrl_dev_config *pcfg;
};

struct comp_data
	uint32_t input_plus;
	uint32_t input_minus;
	uint32_t input_hysteresis;
	uint32_t output_polarity;
	uint32_t output_blanking_source;
};

static void comp_stm32_enable(const struct device *dev)
{
	const struct comp_cfg *cfg = dev->config;

	LL_COMP_Enable(cfg->base);
}

static void comp_stm32_disable(const struct device *dev)
{
	const struct comp_cfg *cfg = dev->config;

	LL_COMP_Disable(cfg->base);
}

#ifdef CONFIG_COMP_LOCK
static void comp_stm32_lock(const struct device *dev)
{
	const struct comp_cfg *cfg = dev->config;

	LL_COMP_Lock(cfg->base);
}
#endif /* CONFIG_COMP_LOCK */

static int comp_stm32_init(const struct device *dev)
{
	const struct comp_cfg *cfg = dev->config;
	const struct comp_data *data = dev->data;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken) != 0) {
		return -EIO;
	}

	int err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (err < 0) {
		LOG_ERR("%s pinctrl setup failed (%d)", cfg->base, err);
		return err;
	}

	LL_COMP_InitTypeDef Comp_InitStruct = { 0 };

	Comp_InitStruct.InputPlus = data->input_plus;
	Comp_InitStruct.InputMinus = data->input_minus;
	Comp_InitStruct.InputHysteresis = data->input_hysteresis;
	Comp_InitStruct.OutputPolarity = data->output_polarity;
	Comp_InitStruct.OutputBlankingSource = data->output_blanking_source;

	err = LL_COMP_Init(cfg->base, &Comp_InitStruct);
	if (err != SUCCESS) {
		LOG_ERR("%s initialisation failed!", cfg->base);
		return -ENODEV;
	}

#ifdef CONFIG_COMP_ENABLE_AT_INIT
	LL_COMP_Enable(cfg->base);
#endif /* CONFIG_COMP_ENABLE_AT_INIT */

#ifdef CONFIG_COMP_LOCK
	LL_COMP_Lock(cfg->base);
#endif /* CONFIG_COMP_LOCK */

	return 0;
}

static const struct comp_driver_api comp_stm32_driver_api = {
	.enable = comp_stm32_enable,
	.disable = comp_stm32_disable,
#ifdef CONFIG_COMP_LOCK
	.lock = comp_stm32_lock,
#endif /* CONFIG_COMP_LOCK */
};


#define STM32_COMP_INIT(index) \
\
	PINCTRL_DT_INST_DEFINE(index); \
\
	static const struct comp_cfg comp_stm32_cfg_##index = { \
		.base = (COMP_TypeDef *)DT_INST_REG_ADDR(index), \
		.pclken = { \
			.enr = DT_INST_CLOCKS_CELL(index, bits), \
			.bus = DT_INST_CLOCKS_CELL(index, bus), \
		}, \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index), \
	}; \
\
	static struct comp_data comp_stm32_data_##index = { \
		.input_plus = DT_INST_PROP_OR(index, input_plus, LL_COMP_INPUT_PLUS_IO1), \
		.input_minus = DT_INST_PROP_OR(index, input_minus, LL_COMP_INPUT_MINUS_IO1), \
		.input_hysteresis = \
			DT_INST_PROP_OR(index, input_hysteresis, LL_COMP_HYSTERESIS_NONE), \
		.output_polarity = \
			DT_INST_PROP_OR(index, output_polarity, LL_COMP_OUTPUTPOL_NONINVERTED), \
		.output_blanking_source = \
			DT_INST_PROP_OR(index, output_blanking_source, LL_COMP_BLANKINGSRC_NONE) \
	}; \
\
	DEVICE_DT_INST_DEFINE(index, &comp_stm32_init, NULL, &comp_stm32_data_##index, \
						  &comp_stm32_cfg_##index, POST_KERNEL, \
						  CONFIG_COMP_INIT_PRIORITY, \
						  &comp_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_COMP_INIT)

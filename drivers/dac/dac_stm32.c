/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dac

#include <errno.h>

#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>
#include <stm32_ll_dac.h>

#define LOG_LEVEL CONFIG_DAC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dac_stm32);

#include <zephyr/drivers/clock_control/stm32_clock_control.h>

/* some low-end MCUs have DAC with only one channel */
#ifdef LL_DAC_CHANNEL_2
#define STM32_CHANNEL_COUNT		2
#else
#define STM32_CHANNEL_COUNT		1
#endif

/* first channel always named 1 */
#define STM32_FIRST_CHANNEL		1

#define CHAN(n)		LL_DAC_CHANNEL_##n
static const uint32_t table_channels[] = {
	CHAN(1),
#ifdef LL_DAC_CHANNEL_2
	CHAN(2),
#endif
};

/* Read-only driver configuration */
struct dac_stm32_cfg {
	/* DAC instance. */
	DAC_TypeDef *base;
	/* Clock configuration. */
	struct stm32_pclken pclken;
	/* pinctrl configurations. */
	const struct pinctrl_dev_config *pcfg;
};

/* Runtime driver data */
struct dac_stm32_data {
	uint8_t channel_count;
	uint8_t resolution;
};

static int dac_stm32_write_value(const struct device *dev,
					uint8_t channel, uint32_t value)
{
	struct dac_stm32_data *data = dev->data;
	const struct dac_stm32_cfg *cfg = dev->config;

	if (channel - STM32_FIRST_CHANNEL >= data->channel_count ||
					channel < STM32_FIRST_CHANNEL) {
		LOG_ERR("Channel %d is not valid", channel);
		return -EINVAL;
	}

	if (value >= BIT(data->resolution)) {
		LOG_ERR("Value %d is out of range", value);
		return -EINVAL;
	}

	if (data->resolution == 8) {
		LL_DAC_ConvertData8RightAligned(cfg->base,
			table_channels[channel - STM32_FIRST_CHANNEL], value);
	} else if (data->resolution == 12) {
		LL_DAC_ConvertData12RightAligned(cfg->base,
			table_channels[channel - STM32_FIRST_CHANNEL], value);
	}

	return 0;
}

static int dac_stm32_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	struct dac_stm32_data *data = dev->data;
	const struct dac_stm32_cfg *cfg = dev->config;
	uint32_t output_buffer;

	if ((channel_cfg->channel_id - STM32_FIRST_CHANNEL >=
			data->channel_count) ||
			(channel_cfg->channel_id < STM32_FIRST_CHANNEL)) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if ((channel_cfg->resolution == 8) ||
			(channel_cfg->resolution == 12)) {
		data->resolution = channel_cfg->resolution;
	} else {
		LOG_ERR("Resolution not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->buffered) {
		output_buffer = LL_DAC_OUTPUT_BUFFER_ENABLE;
	} else {
		output_buffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
	}

	LL_DAC_SetOutputBuffer(cfg->base,
		table_channels[channel_cfg->channel_id - STM32_FIRST_CHANNEL],
		output_buffer);

	LL_DAC_Enable(cfg->base,
		table_channels[channel_cfg->channel_id - STM32_FIRST_CHANNEL]);

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static int dac_stm32_init(const struct device *dev)
{
	const struct dac_stm32_cfg *cfg = dev->config;
	int err;

	/* enable clock for subsystem */
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk,
			     (clock_control_subsys_t) &cfg->pclken) != 0) {
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("DAC pinctrl setup failed (%d)", err);
		return err;
	}

	return 0;
}

static const struct dac_driver_api api_stm32_driver_api = {
	.channel_setup = dac_stm32_channel_setup,
	.write_value = dac_stm32_write_value
};


#define STM32_DAC_INIT(index)						\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
static const struct dac_stm32_cfg dac_stm32_cfg_##index = {		\
	.base = (DAC_TypeDef *)DT_INST_REG_ADDR(index),			\
	.pclken = {							\
		.enr = DT_INST_CLOCKS_CELL(index, bits),		\
		.bus = DT_INST_CLOCKS_CELL(index, bus),			\
	},								\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
};									\
									\
static struct dac_stm32_data dac_stm32_data_##index = {			\
	.channel_count = STM32_CHANNEL_COUNT				\
};									\
									\
DEVICE_DT_INST_DEFINE(index, &dac_stm32_init, NULL,			\
		    &dac_stm32_data_##index,				\
		    &dac_stm32_cfg_##index, POST_KERNEL,		\
		    CONFIG_DAC_INIT_PRIORITY,				\
		    &api_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_DAC_INIT)

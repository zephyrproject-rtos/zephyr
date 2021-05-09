/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dac

#include <errno.h>

#include <drivers/dac.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <stm32_ll_dac.h>

#define LOG_LEVEL CONFIG_DAC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dac_stm32);

#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>

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
	const struct soc_gpio_pinctrl *pinctrl;
	/* Number of pinctrl configurations. */
	size_t pinctrl_len;
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

	/* enable output buffer by default */
	LL_DAC_SetOutputBuffer(cfg->base,
		table_channels[channel_cfg->channel_id - STM32_FIRST_CHANNEL],
		LL_DAC_OUTPUT_BUFFER_ENABLE);

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
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (clock_control_on(clk,
			     (clock_control_subsys_t *) &cfg->pclken) != 0) {
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	err = stm32_dt_pinctrl_configure(cfg->pinctrl,
					 cfg->pinctrl_len,
					 (uint32_t)cfg->base);
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
static const struct soc_gpio_pinctrl dac_pins_##index[] =		\
	ST_STM32_DT_INST_PINCTRL(index, 0);				\
									\
static const struct dac_stm32_cfg dac_stm32_cfg_##index = {		\
	.base = (DAC_TypeDef *)DT_INST_REG_ADDR(index),			\
	.pclken = {							\
		.enr = DT_INST_CLOCKS_CELL(index, bits),		\
		.bus = DT_INST_CLOCKS_CELL(index, bus),			\
	},								\
	.pinctrl = dac_pins_##index,					\
	.pinctrl_len = ARRAY_SIZE(dac_pins_##index),			\
};									\
									\
static struct dac_stm32_data dac_stm32_data_##index = {			\
	.channel_count = STM32_CHANNEL_COUNT				\
};									\
									\
DEVICE_DT_INST_DEFINE(index, &dac_stm32_init, device_pm_control_nop,	\
		    &dac_stm32_data_##index,				\
		    &dac_stm32_cfg_##index, POST_KERNEL,		\
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
		    &api_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_DAC_INIT)

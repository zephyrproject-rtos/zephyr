/*
 * Copyright (c) 2021 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_dac

#include <errno.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/dac.h>

#include <gd32_dac.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dac_gd32, CONFIG_DAC_LOG_LEVEL);

/**
 * For some gd32 series which only have 1 DAC, their HAL name may not same as others.
 * Below definitions help to unify the HAL name.
 */
#if defined(CONFIG_SOC_SERIES_GD32A50X)
#define DAC_CTL_DEN0 DAC_CTL_DEN
#define DAC0_R8DH    OUT_R8DH
#define DAC0_R12DH   OUT_R12DH
#elif defined(CONFIG_SOC_SERIES_GD32F3X0)
#define DAC_CTL_DEN0 DAC_CTL_DEN
#define DAC0_R8DH    DAC_R8DH
#define DAC0_R12DH   DAC_R12DH
#endif

struct dac_gd32_config {
	uint32_t reg;
	uint16_t clkid;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pcfg;
	uint32_t num_channels;
	uint32_t reset_val;
};

struct dac_gd32_data {
	uint8_t resolutions[2];
};

static void dac_gd32_enable(uint8_t dacx)
{
	switch (dacx) {
	case 0U:
		DAC_CTL |= DAC_CTL_DEN0;
		break;
#if DT_INST_PROP(0, num_channels) == 2
	case 1U:
		DAC_CTL |= DAC_CTL_DEN1;
		break;
#endif
	}
}

static void dac_gd32_disable(uint8_t dacx)
{
	switch (dacx) {
	case 0U:
		DAC_CTL &= ~DAC_CTL_DEN0;
		break;
#if DT_INST_PROP(0, num_channels) == 2
	case 1U:
		DAC_CTL &= ~DAC_CTL_DEN1;
		break;
#endif
	}
}

static void dac_gd32_write(struct dac_gd32_data *data,
			   uint8_t dacx, uint32_t value)
{
	switch (dacx) {
	case 0U:
		if (data->resolutions[dacx] == 8U) {
			DAC0_R8DH = value;
		} else {
			DAC0_R12DH = value;
		}
		break;
#if DT_INST_PROP(0, num_channels) == 2
	case 1U:
		if (data->resolutions[dacx] == 8U) {
			DAC1_R8DH = value;
		} else {
			DAC1_R12DH = value;
		}
		break;
#endif
	}
}

static int dac_gd32_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	struct dac_gd32_data *data = dev->data;
	const struct dac_gd32_config *config = dev->config;
	uint8_t dacx = channel_cfg->channel_id;

	if (dacx >= config->num_channels) {
		return -ENOTSUP;
	}

	/* GD32 DAC only support 8 or 12 bits resolution */
	if ((channel_cfg->resolution != 8U) &&
	    (channel_cfg->resolution != 12U)) {
		LOG_ERR("Only 8 and 12 bits resolutions are supported!");
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	data->resolutions[dacx] = channel_cfg->resolution;

	dac_gd32_disable(dacx);
	dac_gd32_write(data, dacx, config->reset_val);
	dac_gd32_enable(dacx);

	return 0;
}

static int dac_gd32_write_value(const struct device *dev,
				uint8_t dacx, uint32_t value)
{
	struct dac_gd32_data *data = dev->data;
	const struct dac_gd32_config *config = dev->config;

	if (dacx >= config->num_channels) {
		return -ENOTSUP;
	}

	dac_gd32_write(data, dacx, value);

	return 0;
}

struct dac_driver_api dac_gd32_driver_api = {
	.channel_setup = dac_gd32_channel_setup,
	.write_value = dac_gd32_write_value
};

static int dac_gd32_init(const struct device *dev)
{
	const struct dac_gd32_config *cfg = dev->config;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clkid);

	(void)reset_line_toggle_dt(&cfg->reset);

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

static struct dac_gd32_data dac_gd32_data_0;

static const struct dac_gd32_config dac_gd32_cfg_0 = {
	.reg = DT_INST_REG_ADDR(0),
	.clkid = DT_INST_CLOCKS_CELL(0, id),
	.reset = RESET_DT_SPEC_INST_GET(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.num_channels = DT_INST_PROP(0, num_channels),
	.reset_val = DT_INST_PROP(0, reset_val),
};

DEVICE_DT_INST_DEFINE(0, &dac_gd32_init, NULL, &dac_gd32_data_0,
		      &dac_gd32_cfg_0, POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,
		      &dac_gd32_driver_api);

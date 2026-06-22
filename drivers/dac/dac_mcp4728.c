/*
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp4728

#include <zephyr/kernel.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_mcp4728, CONFIG_DAC_LOG_LEVEL);

#define MCP4728_MAX_CHANNEL			4U
#define MCP4728_RESOLUTION			0xC
#define MCP4728_DAC_MAX_VAL			((1U << MCP4728_RESOLUTION) - 1U)
#define MCP4728_MULTI_WRITE_CMD_VAL		8U
#define MCP4728_MULTI_WRITE_CMD_POS		3U
#define MCP4728_MULTI_WRITE_CHANNEL_POS		1U
#define MCP4728_MULTI_WRITE_REFERENCE_POS	7U
#define MCP4728_MULTI_WRITE_POWER_DOWN_POS	5U
#define MCP4728_MULTI_WRITE_POWER_DOWN_MASK	0X3
#define MCP4728_MULTI_WRITE_GAIN_POS		4U
#define MCP4728_MULTI_WRITE_DAC_UPPER_VAL_POS	8U
#define MCP4728_MULTI_WRITE_DAC_UPPER_VAL_MASK	0xF
#define MCP4728_MULTI_WRITE_DAC_LOWER_VAL_MASK	0xFF

struct mcp4728_config {
	struct i2c_dt_spec bus;
	uint8_t power_down[MCP4728_MAX_CHANNEL];
	uint8_t voltage_reference[MCP4728_MAX_CHANNEL];
	uint8_t gain[MCP4728_MAX_CHANNEL];
};

static int mcp4728_channel_setup(const struct device *dev,
				const struct dac_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id >= MCP4728_MAX_CHANNEL) {
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != MCP4728_RESOLUTION) {
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		return -ENOTSUP;
	}

	return 0;
}

static int mcp4728_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct mcp4728_config *config = (struct mcp4728_config *)dev->config;
	uint8_t tx_data[3];
	int ret;

	if (channel >= MCP4728_MAX_CHANNEL) {
		return -ENOTSUP;
	}

	if (value > MCP4728_DAC_MAX_VAL) {
		return -ENOTSUP;
	}

	/* Multi-Write Command: Write Multiple DAC Input Registers. */
	tx_data[0] = (MCP4728_MULTI_WRITE_CMD_VAL << MCP4728_MULTI_WRITE_CMD_POS) |
			(channel << MCP4728_MULTI_WRITE_CHANNEL_POS);
	tx_data[1] = (config->voltage_reference[channel] << MCP4728_MULTI_WRITE_REFERENCE_POS) |
			((config->power_down[channel] & MCP4728_MULTI_WRITE_POWER_DOWN_MASK)
			<< MCP4728_MULTI_WRITE_POWER_DOWN_POS) |
			((config->gain[channel] << MCP4728_MULTI_WRITE_GAIN_POS)) |
			((value >> MCP4728_MULTI_WRITE_DAC_UPPER_VAL_POS) &
			MCP4728_MULTI_WRITE_DAC_UPPER_VAL_MASK);
	tx_data[2] = (value & MCP4728_MULTI_WRITE_DAC_LOWER_VAL_MASK);
	ret = i2c_write_dt(&config->bus, tx_data, sizeof(tx_data));

	return ret;
}

static int dac_mcp4728_init(const struct device *dev)
{
	const struct mcp4728_config *config = dev->config;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("%s device not found", config->bus.bus->name);
		return -ENODEV;
	}
	return 0;
}

static DEVICE_API(dac, mcp4728_driver_api) = {
	.channel_setup = mcp4728_channel_setup,
	.write_value = mcp4728_write_value,
};

#define INST_DT_MCP4728(index)							\
	static const struct mcp4728_config mcp4728_config_##index = {		\
		.bus = I2C_DT_SPEC_INST_GET(index),				\
		.power_down = DT_INST_PROP(index, power_down_mode),		\
		.voltage_reference = DT_INST_PROP(index, voltage_reference),	\
		.gain = DT_INST_PROP_OR(index, gain, {0}),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(index, dac_mcp4728_init, NULL, NULL,		\
				&mcp4728_config_##index,			\
				POST_KERNEL,					\
				CONFIG_DAC_MCP4728_INIT_PRIORITY,		\
				&mcp4728_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INST_DT_MCP4728);

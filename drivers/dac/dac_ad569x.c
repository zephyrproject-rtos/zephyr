/*
 * Copyright (c) 2024 Jan Kubiznak <jan.kubiznak@deveritec.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(dac_ad569x, CONFIG_DAC_LOG_LEVEL);

#define AD569X_CTRL_GAIN(x)  FIELD_PREP(BIT(11), x)
#define AD569X_CTRL_REF(x)   FIELD_PREP(BIT(12), x)
#define AD569X_CTRL_PD(x)    FIELD_PREP(BIT_MASK(2) << 13, x)
#define AD569X_CTRL_RESET(x) FIELD_PREP(BIT(15), x)

#define AD569X_CMD_WRITE            0x10
#define AD569X_CMD_UPDATE           0x20
#define AD569X_CMD_WRITE_AND_UPDATE 0x30
#define AD569X_CMD_CONFIGURE        0x40

#define AD569X_CTRL_NO_RESET      0x00
#define AD569X_CTRL_PERFORM_RESET 0x01

struct ad569x_config {
	struct i2c_dt_spec bus;
	uint8_t resolution;
	uint8_t gain;
	uint8_t voltage_reference;
	uint8_t power_down_mode;
};

static int ad569x_write(const struct device *dev, uint8_t command, uint16_t value)
{
	const struct ad569x_config *config = dev->config;

	uint8_t tx_data[3];

	tx_data[0] = command;
	sys_put_be16(value, tx_data + 1);

	return i2c_write_dt(&config->bus, tx_data, sizeof(tx_data));
}

static int ad569x_read(const struct device *dev, uint16_t *value)
{
	const struct ad569x_config *config = dev->config;

	uint8_t rx_data[2];
	int ret;

	ret = i2c_read_dt(&config->bus, rx_data, sizeof(rx_data));
	if (ret != 0) {
		return ret;
	}

	*value = sys_get_be16(rx_data);

	return ret;
}

static int ad569x_channel_setup(const struct device *dev, const struct dac_channel_cfg *channel_cfg)
{
	const struct ad569x_config *config = dev->config;

	if (channel_cfg->channel_id > 0) {
		LOG_ERR("invalid channel %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("invalid resolution %d", channel_cfg->resolution);
		return -EINVAL;
	}

	return 0;
}

static int ad569x_sw_reset(const struct device *dev)
{
	uint16_t reg = AD569X_CTRL_RESET(AD569X_CTRL_PERFORM_RESET);
	int ret;

	LOG_DBG("reset %s", dev->name);

	/* Ignore return value, since device gives NAK after receiving RESET request */
	ad569x_write(dev, AD569X_CMD_CONFIGURE, reg);

	/* Check that DAC output is reset */
	ret = ad569x_read(dev, &reg);
	if (ret != 0) {
		LOG_ERR("failed to read value");
		return ret;
	}

	if (reg != 0) {
		LOG_ERR("failed to reset DAC output");
		ret = -EIO;
	}

	return 0;
}

static int ad569x_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct ad569x_config *config = dev->config;

	if (channel > 0) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	if (value > (BIT(config->resolution) - 1)) {
		LOG_ERR("invalid value %d", value);
		return -EINVAL;
	}

	return ad569x_write(dev, AD569X_CMD_WRITE_AND_UPDATE, value);
}

static int ad569x_init(const struct device *dev)
{
	const struct ad569x_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	ret = ad569x_sw_reset(dev);
	if (ret != 0) {
		LOG_ERR("failed to perform sw reset");
		return ret;
	}

	LOG_DBG("configure %s: gain %d, voltage reference %d, power down mode %d", dev->name,
		config->gain, config->voltage_reference, config->power_down_mode);

	uint16_t ctrl_reg = AD569X_CTRL_GAIN(config->gain) |
			    AD569X_CTRL_REF(config->voltage_reference) |
			    AD569X_CTRL_PD(config->power_down_mode);

	ret = ad569x_write(dev, AD569X_CMD_CONFIGURE, ctrl_reg);
	if (ret != 0) {
		LOG_ERR("failed to configure the device");
		return ret;
	}

	return 0;
}

static const struct dac_driver_api ad569x_driver_api = {
	.channel_setup = ad569x_channel_setup,
	.write_value = ad569x_write_value,
};

#define INST_DT_AD569X(index, name, res)                                                           \
	static const struct ad569x_config config_##name##_##index = {                              \
		.bus = I2C_DT_SPEC_INST_GET(index),                                                \
		.resolution = res,                                                                 \
		.gain = DT_INST_ENUM_IDX(index, gain),                                             \
		.voltage_reference = DT_INST_ENUM_IDX(index, voltage_reference),                   \
		.power_down_mode = DT_INST_ENUM_IDX(index, power_down_mode),                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, ad569x_init, NULL, NULL, &config_##name##_##index,            \
			      POST_KERNEL, CONFIG_DAC_INIT_PRIORITY, &ad569x_driver_api);

#define DT_DRV_COMPAT adi_ad5691
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define DAC_AD5691_RESOLUTION 12
DT_INST_FOREACH_STATUS_OKAY_VARGS(INST_DT_AD569X, DT_DRV_COMPAT, DAC_AD5691_RESOLUTION)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5692
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define DAC_AD5692_RESOLUTION 14
DT_INST_FOREACH_STATUS_OKAY_VARGS(INST_DT_AD569X, DT_DRV_COMPAT, DAC_AD5692_RESOLUTION)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_ad5693
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define DAC_AD5693_RESOLUTION 16
DT_INST_FOREACH_STATUS_OKAY_VARGS(INST_DT_AD569X, DT_DRV_COMPAT, DAC_AD5693_RESOLUTION)
#endif
#undef DT_DRV_COMPAT

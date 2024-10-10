/*
 * Copyright (c) 2020 Matija Tudan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_dacx3608, CONFIG_DAC_LOG_LEVEL);

/* Register addresses */
#define DACX3608_REG_DEVICE_CONFIG  0x01U
#define DACX3608_REG_STATUS_TRIGGER 0x02U
#define DACX3608_REG_BRDCAST        0x03U
#define DACX3608_REG_DACA_DATA      0x08U

#define DAC43608_DEVICE_ID      0x500	/* STATUS_TRIGGER[DEVICE_ID] */
#define DAC53608_DEVICE_ID      0x300	/* STATUS_TRIGGER[DEVICE_ID] */
#define DACX3608_SW_RST         0x0A	/* STATUS_TRIGGER[SW_RST] */
#define DACX3608_POR_DELAY      5
#define DACX3608_MAX_CHANNEL    8

struct dacx3608_config {
	struct i2c_dt_spec bus;
	uint8_t resolution;
};

struct dacx3608_data {
	uint8_t configured;
};

static int dacx3608_reg_read(const struct device *dev, uint8_t reg,
			      uint16_t *val)
{
	const struct dacx3608_config *cfg = dev->config;

	if (i2c_burst_read_dt(&cfg->bus, reg, (uint8_t *) val, 2) < 0) {
		LOG_ERR("I2C read failed");
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int dacx3608_reg_write(const struct device *dev, uint8_t reg,
			       uint16_t val)
{
	const struct dacx3608_config *cfg = dev->config;
	uint8_t buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

int dacx3608_reg_update(const struct device *dev, uint8_t reg,
			 uint16_t mask, bool setting)
{
	uint16_t regval;
	int ret;

	ret = dacx3608_reg_read(dev, reg, &regval);
	if (ret) {
		return -EIO;
	}

	if (setting) {
		regval |= mask;
	} else {
		regval &= ~mask;
	}

	ret = dacx3608_reg_write(dev, reg, regval);
	if (ret) {
		return ret;
	}

	return 0;
}

static int dacx3608_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	const struct dacx3608_config *config = dev->config;
	struct dacx3608_data *data = dev->data;
	bool setting = false;
	int ret;

	if (channel_cfg->channel_id > DACX3608_MAX_CHANNEL - 1) {
		LOG_ERR("Unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	if (data->configured & BIT(channel_cfg->channel_id)) {
		LOG_DBG("Channel %d already configured", channel_cfg->channel_id);
		return 0;
	}

	/* Clear PDNn bit */
	ret = dacx3608_reg_update(dev, DACX3608_REG_DEVICE_CONFIG,
				BIT(channel_cfg->channel_id), setting);
	if (ret) {
		LOG_ERR("Unable to update DEVICE_CONFIG register");
		return -EIO;
	}

	data->configured |= BIT(channel_cfg->channel_id);

	LOG_DBG("Channel %d initialized", channel_cfg->channel_id);

	return 0;
}

static int dacx3608_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	const struct dacx3608_config *config = dev->config;
	struct dacx3608_data *data = dev->data;
	uint16_t regval;
	int ret;

	if (channel > DACX3608_MAX_CHANNEL - 1) {
		LOG_ERR("Unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (!(data->configured & BIT(channel))) {
		LOG_ERR("Channel %d not initialized", channel);
		return -EINVAL;
	}

	if (value >= (1 << (config->resolution))) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	/*
	 * Shift passed value two times left because first two bits are Don't Care
	 *
	 * DACn_DATA register format:
	 *
	 * | 15 14 13 12 |      11 10 9 8 7 6 5 4 3 2      |    1 0     |
	 * |-------------|---------------------------------|------------|
	 * | Don't Care  |  DAC53608[9:0] / DAC43608[7:0]  | Don't Care |
	 */
	regval = value << 2;
	regval &= 0xFFFF;

	ret = dacx3608_reg_write(dev, DACX3608_REG_DACA_DATA + channel, regval);
	if (ret) {
		LOG_ERR("Unable to set value %d on channel %d", value, channel);
		return -EIO;
	}

	return 0;
}

static int dacx3608_soft_reset(const struct device *dev)
{
	uint16_t regval = DACX3608_SW_RST;
	int ret;

	ret = dacx3608_reg_write(dev, DACX3608_REG_STATUS_TRIGGER, regval);
	if (ret) {
		return -EIO;
	}
	k_msleep(DACX3608_POR_DELAY);

	return 0;
}

static int dacx3608_device_id_check(const struct device *dev)
{
	uint16_t dev_id;
	int ret;

	ret = dacx3608_reg_read(dev, DACX3608_REG_STATUS_TRIGGER, &dev_id);
	if (ret) {
		LOG_ERR("Unable to read device ID");
		return -EIO;
	}

	switch (dev_id) {
	case DAC43608_DEVICE_ID:
	case DAC53608_DEVICE_ID:
		LOG_DBG("Device ID %#4x", dev_id);
		break;
	default:
		LOG_ERR("Unknown Device ID %#4x", dev_id);
		return -EIO;
	}

	return 0;
}

static int dacx3608_init(const struct device *dev)
{
	const struct dacx3608_config *config = dev->config;
	struct dacx3608_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	ret = dacx3608_soft_reset(dev);
	if (ret) {
		LOG_ERR("Soft-reset failed");
		return ret;
	}

	ret = dacx3608_device_id_check(dev);
	if (ret) {
		return ret;
	}

	data->configured = 0;

	LOG_DBG("Init complete");

	return 0;
}

static const struct dac_driver_api dacx3608_driver_api = {
	.channel_setup = dacx3608_channel_setup,
	.write_value = dacx3608_write_value,
};

#define INST_DT_DACX3608(inst, t) DT_INST(inst, ti_dac##t)

#define DACX3608_DEVICE(t, n, res) \
	static struct dacx3608_data dac##t##_data_##n; \
	static const struct dacx3608_config dac##t##_config_##n = { \
		.bus = I2C_DT_SPEC_GET(INST_DT_DACX3608(n, t)), \
		.resolution = res, \
	}; \
	DEVICE_DT_DEFINE(INST_DT_DACX3608(n, t), \
				&dacx3608_init, NULL, \
				&dac##t##_data_##n, \
				&dac##t##_config_##n, POST_KERNEL, \
				CONFIG_DAC_DACX3608_INIT_PRIORITY, \
				&dacx3608_driver_api)

/*
 * DAC43608: 8-bit
 */
#define DAC43608_DEVICE(n) DACX3608_DEVICE(43608, n, 8)

/*
 * DAC53608: 10-bit
 */
#define DAC53608_DEVICE(n) DACX3608_DEVICE(53608, n, 10)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_DACX3608_FOREACH(t, inst_expr) \
	LISTIFY(DT_NUM_INST_STATUS_OKAY(ti_dac##t), \
		     CALL_WITH_ARG, (), inst_expr)

INST_DT_DACX3608_FOREACH(43608, DAC43608_DEVICE);
INST_DT_DACX3608_FOREACH(53608, DAC53608_DEVICE);

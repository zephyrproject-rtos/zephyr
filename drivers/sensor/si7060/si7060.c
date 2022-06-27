/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si7060

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "si7060.h"

#define SIGN_BIT_MASK 0x7F

LOG_MODULE_REGISTER(si7060, CONFIG_SENSOR_LOG_LEVEL);

struct si7060_data {
	uint16_t temperature;
};

struct si7060_config {
	struct i2c_dt_spec i2c;
};

static int si7060_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct si7060_config *config = dev->config;

	if (i2c_reg_read_byte_dt(&config->i2c, reg, val)) {
		return -EIO;
	}

	return 0;
}

static int si7060_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct si7060_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, val);
}

static int si7060_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct si7060_data *drv_data = dev->data;

	if (si7060_reg_write(dev, SI7060_REG_CONFIG,
			     SI7060_ONE_BURST_VALUE) != 0) {
		return -EIO;
	}

	int retval;
	uint8_t dspsigm;
	uint8_t dspsigl;

	retval = si7060_reg_read(dev, SI7060_REG_TEMP_HIGH, &dspsigm);
	retval += si7060_reg_read(dev, SI7060_REG_TEMP_LOW, &dspsigl);

	if (retval == 0) {
		drv_data->temperature = (256 * (dspsigm & SIGN_BIT_MASK))
		+ dspsigl;
	} else {
		LOG_ERR("Read register err");
	}

	LOG_DBG("Sample_fetch retval: %d", retval);

	return retval;
}

static int si7060_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct si7060_data *drv_data = dev->data;
	int32_t uval;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		uval = ((55 * 160) + (drv_data->temperature - 16384)) >> 4;
		val->val1 = uval / 10;
		val->val2 = (uval % 10) * 100000;

		LOG_DBG("Temperature = val1:%d, val2:%d", val->val1, val->val2);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api si7060_api = {
	.sample_fetch = &si7060_sample_fetch,
	.channel_get = &si7060_channel_get,
};

static int si7060_chip_init(const struct device *dev)
{
	const struct si7060_config *config = dev->config;
	uint8_t value;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (si7060_reg_read(dev, SI7060_REG_CHIP_INFO, &value) != 0) {
		return -EIO;
	}

	if ((value >> 4) != SI7060_CHIP_ID_VALUE) {
		LOG_ERR("Bad chip id 0x%x", value);
		return -ENOTSUP;
	}

	return 0;
}

static int si7060_init(const struct device *dev)
{
	if (si7060_chip_init(dev) < 0) {
		return -EINVAL;
	}

	return 0;
}

static struct si7060_data si_data;

static const struct si7060_config si7060_config_inst = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
};

DEVICE_DT_INST_DEFINE(0, si7060_init, NULL, &si_data, &si7060_config_inst,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7060_api);

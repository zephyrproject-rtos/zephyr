/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max44009

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "max44009.h"

LOG_MODULE_REGISTER(MAX44009, CONFIG_SENSOR_LOG_LEVEL);

static int max44009_reg_read(const struct device *dev, uint8_t reg,
			     uint8_t *val, bool send_stop)
{
	const struct max44009_config *config = dev->config;
	struct i2c_msg msgs[2] = {
		{
			.buf = &reg,
			.len = 1,
			.flags = I2C_MSG_WRITE,
		},
		{
			.buf = val,
			.len = 1,
			.flags = I2C_MSG_READ,
		},
	};

	if (send_stop) {
		msgs[1].flags |= I2C_MSG_STOP;
	}

	if (i2c_transfer_dt(&config->i2c, msgs, 2) != 0) {
		return -EIO;
	}

	return 0;
}

static int max44009_reg_write(const struct device *dev, uint8_t reg,
			      uint8_t val)
{
	const struct max44009_config *config = dev->config;
	uint8_t tx_buf[2] = {reg, val};

	return i2c_write_dt(&config->i2c, tx_buf, sizeof(tx_buf));
}

static int max44009_reg_update(const struct device *dev, uint8_t reg,
			       uint8_t mask, uint8_t val)
{
	uint8_t old_val = 0U;
	uint8_t new_val = 0U;

	if (max44009_reg_read(dev, reg, &old_val, true) != 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return max44009_reg_write(dev, reg, new_val);
}

static int max44009_attr_set(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	uint8_t value;
	uint32_t cr;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		/* convert rate to mHz */
		cr = val->val1 * 1000 + val->val2 / 1000;

		/* the sensor supports 1.25Hz or continuous conversion */
		switch (cr) {
		case 1250:
			value = 0U;
			break;
		default:
			value = MAX44009_CONTINUOUS_SAMPLING;
		}

		if (max44009_reg_update(dev, MAX44009_REG_CONFIG,
					MAX44009_SAMPLING_CONTROL_BIT,
					value) != 0) {
			LOG_DBG("Failed to set attribute!");
			return -EIO;
		}

		return 0;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int max44009_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct max44009_data *drv_data = dev->data;
	uint8_t val_h, val_l;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT);

	drv_data->sample = 0U;

	if (max44009_reg_read(dev, MAX44009_REG_LUX_HIGH_BYTE, &val_h,
			      false) != 0) {
		return -EIO;
	}

	if (max44009_reg_read(dev, MAX44009_REG_LUX_LOW_BYTE, &val_l,
			      true) != 0) {
		return -EIO;
	}

	drv_data->sample = ((uint16_t)val_h) << 8;
	drv_data->sample += val_l;

	return 0;
}

static int max44009_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max44009_data *drv_data = dev->data;
	uint32_t uval;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	/**
	 * sample consists of 4 bits of exponent and 8 bits of mantissa
	 * bits 15 to 12 are exponent bits
	 * bits 11 to 8 and 3 to 0 are the mantissa bits
	 */
	uval = drv_data->sample;
	uval = (uval & MAX44009_MANTISSA_LOW_NIBBLE_MASK) +
	       ((uval & MAX44009_MANTISSA_HIGH_NIBBLE_MASK) >> 4);
	uval = uval << (drv_data->sample >> MAX44009_SAMPLE_EXPONENT_SHIFT);

	/* lux is the integer of sample output multiplied by 0.045. */
	val->val1 = (uval * 45U) / 1000;
	val->val2 = ((uval * 45U) % 1000) * 1000U;

	return 0;
}

static DEVICE_API(sensor, max44009_driver_api) = {
	.attr_set = max44009_attr_set,
	.sample_fetch = max44009_sample_fetch,
	.channel_get = max44009_channel_get,
};

int max44009_init(const struct device *dev)
{
	const struct max44009_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return 0;
}

#define MAX44009_DEFINE(inst)									\
	static struct max44009_data max44009_data_##inst;					\
												\
	static const struct max44009_config max44009_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, max44009_init, NULL,					\
			      &max44009_data_##inst, &max44009_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &max44009_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(MAX44009_DEFINE)

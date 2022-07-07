/*
 * Copyright (c) 2021 Pete Dietl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max31875

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAX31875, CONFIG_SENSOR_LOG_LEVEL);

/* Conversions per second */
#define MAX31875_CONV_PER_SEC_SHIFT 0x01
#define MAX31875_CONV_PER_SEC_0_25 0x00
#define MAX31875_CONV_PER_SEC_1    0x01
#define MAX31875_CONV_PER_SEC_4    0x02
#define MAX31875_CONV_PER_SEC_8    0x03

#define MAX31875_CONV_PER_SEC_MASK (BIT_MASK(2) << MAX31875_CONV_PER_SEC_SHIFT)

/* Data format */
#define MAX31875_DATA_FORMAT_SHIFT 0x07

#define MAX31875_DATA_FORMAT_NORMAL   0x00
#define MAX31875_DATA_FORMAT_EXTENDED 0x01

/* These are for shifting the data received */
#define MAX31875_DATA_FORMAT_EXTENDED_SHIFT 0x03
#define MAX31875_DATA_FORMAT_NORMAL_SHIFT   0x04

/* Resolution in bits */
#define MAX31875_RESOLUTION_SHIFT 0x05

#define MAX31875_RESOLUTION_8_BITS  0x00
#define MAX31875_RESOLUTION_9_BITS  0x01
#define MAX31875_RESOLUTION_10_BITS 0x02
#define MAX31875_RESOLUTION_12_BITS 0x03

#define MAX31875_TEMP_SCALE 62500

/**
 * @brief Macro for creating the MAX31875 configuration register value
 *
 * @param mode  Data format
 * @param res   Resolution (number of bits)
 * @param convs Conversions per second
 */
#define MAX31875_CONFIG(format, res, convs)						       \
	(((format) << (MAX31875_DATA_FORMAT_SHIFT)) | ((res) << (MAX31875_RESOLUTION_SHIFT)) | \
	 ((convs) << (MAX31875_CONV_PER_SEC_SHIFT)))

#define MAX31875_REG_TEMPERATURE 0x00
#define MAX31875_REG_CONFIG      0x01

struct max31875_data {
	int16_t sample;
	uint16_t config_reg;
};

struct max31875_config {
	const struct i2c_dt_spec bus;
	uint8_t conversions_per_second;
	uint8_t data_format;
	uint8_t resolution;
};

static int max31875_reg_read(const struct max31875_config *cfg,
			     uint8_t reg, uint16_t *val)
{
	int ret;

	ret = i2c_burst_read_dt(&cfg->bus, reg, (uint8_t *)val, sizeof(*val));
	if (ret < 0) {
		return ret;
	}

	*val = sys_be16_to_cpu(*val);
	return 0;
}

static int max31875_reg_write(const struct max31875_config *cfg,
			      uint8_t reg, uint16_t val)
{
	uint16_t val_be = sys_cpu_to_be16(val);

	return i2c_burst_write_dt(&cfg->bus, reg, (uint8_t *)&val_be, 2);
}

static uint16_t set_config_flags(struct max31875_data *data, uint16_t mask,
				 uint16_t value)
{
	return (data->config_reg & ~mask) | (value & mask);
}

static int max31875_update_config(const struct device *dev, uint16_t mask, uint16_t val)
{
	int rc;
	const struct max31875_config *cfg = dev->config;
	struct max31875_data *data = dev->data;
	const uint16_t new_val = set_config_flags(data, mask, val);

	rc = max31875_reg_write(cfg, MAX31875_REG_CONFIG, new_val);

	/* don't update if write failed */
	if (rc == 0) {
		data->config_reg = new_val;
	}

	return rc;
}

static int max31875_attr_set(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	int ret;
	uint16_t value;
	uint16_t cr;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		/* the sensor supports two ranges -50 to 128 and -50 to 150 */
		/* the value contains the upper limit */
		if (val->val1 == 128) {
			value = MAX31875_DATA_FORMAT_NORMAL << MAX31875_DATA_FORMAT_SHIFT;
		} else if (val->val1 == 150) {
			value = MAX31875_DATA_FORMAT_EXTENDED << MAX31875_DATA_FORMAT_SHIFT;
		} else {
			return -ENOTSUP;
		}

		ret = max31875_update_config(dev, MAX31875_DATA_FORMAT_SHIFT, value);
		if (ret < 0) {
			LOG_ERR("Failed to set attribute!");
			return ret;
		}
		break;
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		/* conversion rate in mHz */
		cr = val->val1 * 1000 + val->val2 / 1000;

		/* the sensor supports 0.25Hz, 1Hz, 4Hz and 8Hz */
		/* conversion rate */
		switch (cr) {
		case 250:
			value = MAX31875_CONV_PER_SEC_0_25 << MAX31875_CONV_PER_SEC_SHIFT;
			break;

		case 1000:
			value = MAX31875_CONV_PER_SEC_1 << MAX31875_CONV_PER_SEC_SHIFT;
			break;

		case 4000:
			value = MAX31875_CONV_PER_SEC_4 << MAX31875_CONV_PER_SEC_SHIFT;
			break;

		case 8000:
			value = MAX31875_CONV_PER_SEC_8 << MAX31875_CONV_PER_SEC_SHIFT;
			break;

		default:
			return -ENOTSUP;
		}

		ret = max31875_update_config(dev, MAX31875_CONV_PER_SEC_MASK, value);
		if (ret < 0) {
			LOG_ERR("Failed to set attribute!");
			return ret;
		}

		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int max31875_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct max31875_data *data = dev->data;
	const struct max31875_config *cfg = dev->config;
	uint16_t val;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Invalid channel provided");
		return -ENOTSUP;
	}

	ret = max31875_reg_read(cfg, MAX31875_REG_TEMPERATURE, &val);
	if (ret < 0) {
		return ret;
	}

	if (data->config_reg & BIT(MAX31875_DATA_FORMAT_SHIFT)) {
		data->sample = arithmetic_shift_right((int16_t)val,
						      MAX31875_DATA_FORMAT_EXTENDED_SHIFT);
	} else {
		data->sample = arithmetic_shift_right((int16_t)val,
						      MAX31875_DATA_FORMAT_NORMAL_SHIFT);
	}

	return 0;
}

static int max31875_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max31875_data *data = dev->data;
	int32_t uval;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	uval = data->sample * MAX31875_TEMP_SCALE;
	val->val1 = uval / 1000000;
	val->val2 = uval % 1000000;

	return 0;
}

static const struct sensor_driver_api max31875_driver_api = {
	.attr_set = max31875_attr_set,
	.sample_fetch = max31875_sample_fetch,
	.channel_get = max31875_channel_get,
};

static int max31875_init(const struct device *dev)
{
	const struct max31875_config *cfg = dev->config;
	struct max31875_data *data = dev->data;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	data->config_reg = MAX31875_CONFIG(cfg->data_format, cfg->resolution,
					   cfg->conversions_per_second);

	return max31875_update_config(dev, 0, 0);
}


#define MAX31875_INST(inst)								  \
	static struct max31875_data max31875_data_##inst;				  \
	static const struct max31875_config max31875_config_##inst = {			  \
		.bus = I2C_DT_SPEC_INST_GET(inst),					  \
		.conversions_per_second = DT_INST_ENUM_IDX(inst, conversions_per_second), \
		.resolution = DT_INST_ENUM_IDX(inst, resolution),			  \
		.data_format = DT_INST_PROP(inst, extended_mode),			  \
	};										  \
	DEVICE_DT_INST_DEFINE(inst, max31875_init, NULL, &max31875_data_##inst,		  \
			      &max31875_config_##inst, POST_KERNEL,			  \
			      CONFIG_SENSOR_INIT_PRIORITY, &max31875_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31875_INST)

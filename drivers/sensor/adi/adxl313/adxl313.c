/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl313

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "adxl313.h"

LOG_MODULE_REGISTER(ADXL313, CONFIG_SENSOR_LOG_LEVEL);

static int adxl313_reg_write_mask(const struct device *dev, uint8_t reg, uint8_t mask,
				  uint8_t value)
{
	const struct adxl313_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, reg, mask, value);
}

static int adxl313_set_op_mode(const struct device *dev, bool measure)
{
	const struct adxl313_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, ADXL313_REG_POWER_CTL,
				     measure ? ADXL313_POWER_CTL_MEASURE : 0);
}

static int adxl313_set_odr(const struct device *dev, enum adxl313_odr odr)
{
	struct adxl313_data *data = dev->data;
	int ret;

	ret = adxl313_reg_write_mask(dev, ADXL313_REG_BW_RATE, ADXL313_ODR_MSK, odr);
	if (ret < 0) {
		return ret;
	}

	data->odr = odr;
	return 0;
}

static int adxl313_set_range(const struct device *dev, enum adxl313_range range)
{
	struct adxl313_data *data = dev->data;
	int ret;

	ret = adxl313_reg_write_mask(dev, ADXL313_REG_DATA_FORMAT, ADXL313_DATA_FORMAT_RANGE_MSK,
				     range);
	if (ret < 0) {
		return ret;
	}

	data->range = range;
	return 0;
}

static int adxl313_attr_set_odr(const struct device *dev, const struct sensor_value *val)
{
	enum adxl313_odr odr;

	/* Accept integer Hz; 6 and 12 map to 6.25 Hz and 12.5 Hz. */
	switch (val->val1) {
	case 6:
		odr = ADXL313_ODR_6_25HZ;
		break;
	case 12:
		odr = ADXL313_ODR_12_5HZ;
		break;
	case 25:
		odr = ADXL313_ODR_25HZ;
		break;
	case 50:
		odr = ADXL313_ODR_50HZ;
		break;
	case 100:
		odr = ADXL313_ODR_100HZ;
		break;
	case 200:
		odr = ADXL313_ODR_200HZ;
		break;
	case 400:
		odr = ADXL313_ODR_400HZ;
		break;
	case 800:
		odr = ADXL313_ODR_800HZ;
		break;
	case 1600:
		odr = ADXL313_ODR_1600HZ;
		break;
	case 3200:
		odr = ADXL313_ODR_3200HZ;
		break;
	default:
		return -EINVAL;
	}

	return adxl313_set_odr(dev, odr);
}

static int adxl313_attr_set_range(const struct device *dev, int32_t range_g)
{
	enum adxl313_range range;

	/*
	 * FULL_SCALE is expressed in integer g via sensor_ms2_to_g().
	 * 0 maps to ±0.5 g (half-g cannot be represented as a non-zero int).
	 */
	switch (range_g) {
	case 0:
		range = ADXL313_RANGE_0_5G;
		break;
	case 1:
		range = ADXL313_RANGE_1G;
		break;
	case 2:
		range = ADXL313_RANGE_2G;
		break;
	case 4:
		range = ADXL313_RANGE_4G;
		break;
	default:
		return -EINVAL;
	}

	return adxl313_set_range(dev, range);
}

static int adxl313_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	int ret;
	int ret_meas;

	ARG_UNUSED(chan);

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	case SENSOR_ATTR_FULL_SCALE:
		break;
	default:
		return -ENOTSUP;
	}

	/* Datasheet: change BW_RATE / DATA_FORMAT while in standby */
	ret = adxl313_set_op_mode(dev, false);
	if (ret < 0) {
		return ret;
	}

	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		ret = adxl313_attr_set_odr(dev, val);
	} else {
		/*
		 * sensor_ms2_to_g() is integer g. Pass ~0.5 g (rounds to 0) to
		 * select ±0.5 g; 1/2/4 select the matching full-scale ranges.
		 */
		ret = adxl313_attr_set_range(dev, sensor_ms2_to_g(val));
	}

	ret_meas = adxl313_set_op_mode(dev, true);

	/* Prefer the configuration error if measure-mode restore also failed */
	return ret < 0 ? ret : ret_meas;
}

static int adxl313_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	struct adxl313_data *data = dev->data;

	ARG_UNUSED(chan);

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		switch (data->odr) {
		case ADXL313_ODR_6_25HZ:
			val->val1 = 6;
			val->val2 = 250000;
			break;
		case ADXL313_ODR_12_5HZ:
			val->val1 = 12;
			val->val2 = 500000;
			break;
		case ADXL313_ODR_25HZ:
			val->val1 = 25;
			val->val2 = 0;
			break;
		case ADXL313_ODR_50HZ:
			val->val1 = 50;
			val->val2 = 0;
			break;
		case ADXL313_ODR_100HZ:
			val->val1 = 100;
			val->val2 = 0;
			break;
		case ADXL313_ODR_200HZ:
			val->val1 = 200;
			val->val2 = 0;
			break;
		case ADXL313_ODR_400HZ:
			val->val1 = 400;
			val->val2 = 0;
			break;
		case ADXL313_ODR_800HZ:
			val->val1 = 800;
			val->val2 = 0;
			break;
		case ADXL313_ODR_1600HZ:
			val->val1 = 1600;
			val->val2 = 0;
			break;
		case ADXL313_ODR_3200HZ:
			val->val1 = 3200;
			val->val2 = 0;
			break;
		default:
			return -EINVAL;
		}
		return 0;
	case SENSOR_ATTR_FULL_SCALE: {
		int64_t micro_ms2;

		switch (data->range) {
		case ADXL313_RANGE_0_5G:
			micro_ms2 = SENSOR_G / 2;
			break;
		case ADXL313_RANGE_1G:
			micro_ms2 = SENSOR_G;
			break;
		case ADXL313_RANGE_2G:
			micro_ms2 = 2 * SENSOR_G;
			break;
		case ADXL313_RANGE_4G:
			micro_ms2 = 4 * SENSOR_G;
			break;
		default:
			return -EINVAL;
		}

		val->val1 = (int32_t)(micro_ms2 / 1000000);
		val->val2 = (int32_t)(micro_ms2 % 1000000);
		return 0;
	}
	default:
		return -ENOTSUP;
	}
}

static void adxl313_accel_convert(struct sensor_value *val, int16_t sample)
{
	/* Full-resolution: 1024 LSB/g for all ranges */
	int64_t micro_ms2 = ((int64_t)sample * SENSOR_G) / ADXL313_FULL_RES_LSB_PER_G;

	val->val1 = (int32_t)(micro_ms2 / 1000000);
	val->val2 = (int32_t)(micro_ms2 % 1000000);
}

static int adxl313_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct adxl313_config *cfg = dev->config;
	struct adxl313_data *data = dev->data;
	uint8_t axis_data[6];
	int ret;

	ARG_UNUSED(chan);

	/* Read latest sample from data registers (no DATA_READY wait) */
	ret = i2c_burst_read_dt(&cfg->i2c, ADXL313_REG_DATAX0, axis_data, sizeof(axis_data));
	if (ret < 0) {
		LOG_ERR("Failed to read samples: %d", ret);
		return ret;
	}

	data->x = (int16_t)sys_get_le16(&axis_data[0]);
	data->y = (int16_t)sys_get_le16(&axis_data[2]);
	data->z = (int16_t)sys_get_le16(&axis_data[4]);

	return 0;
}

static int adxl313_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl313_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl313_accel_convert(val, data->x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl313_accel_convert(val, data->y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl313_accel_convert(val, data->z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl313_accel_convert(val++, data->x);
		adxl313_accel_convert(val++, data->y);
		adxl313_accel_convert(val, data->z);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, adxl313_api_funcs) = {
	.attr_set = adxl313_attr_set,
	.attr_get = adxl313_attr_get,
	.sample_fetch = adxl313_sample_fetch,
	.channel_get = adxl313_channel_get,
};

static int adxl313_init(const struct device *dev)
{
	const struct adxl313_config *cfg = dev->config;
	struct adxl313_data *data = dev->data;
	uint8_t devid_ad;
	uint8_t devid1;
	uint8_t partid;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADXL313_REG_DEVID_AD, &devid_ad);
	if (ret < 0 || devid_ad != ADXL313_DEVID_AD_VAL) {
		LOG_ERR("Unexpected DEVID_AD 0x%02x (ret=%d)", devid_ad, ret);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADXL313_REG_DEVID1_AD, &devid1);
	if (ret < 0 || devid1 != ADXL313_DEVID1_AD_VAL) {
		LOG_ERR("Unexpected DEVID1 0x%02x (ret=%d)", devid1, ret);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADXL313_REG_PARTID, &partid);
	if (ret < 0 || partid != ADXL313_PARTID_VAL) {
		LOG_ERR("Unexpected PARTID 0x%02x (ret=%d)", partid, ret);
		return -ENODEV;
	}

	/* Soft reset; device returns to standby */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, ADXL313_REG_SOFT_RESET, ADXL313_RESET_KEY);
	if (ret < 0) {
		LOG_ERR("Soft reset failed: %d", ret);
		return ret;
	}
	k_msleep(2);

	/* Full resolution, right-justified, configured range */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, ADXL313_REG_DATA_FORMAT,
				    ADXL313_DATA_FORMAT_FULL_RES | cfg->range);
	if (ret < 0) {
		LOG_ERR("DATA_FORMAT write failed: %d", ret);
		return ret;
	}
	data->range = cfg->range;

	ret = adxl313_set_odr(dev, cfg->odr);
	if (ret < 0) {
		LOG_ERR("ODR configuration failed: %d", ret);
		return ret;
	}

	ret = adxl313_set_op_mode(dev, true);
	if (ret < 0) {
		LOG_ERR("Failed to enter measure mode: %d", ret);
		return ret;
	}

	return 0;
}

#define ADXL313_DEFINE(inst)                                                                       \
	static struct adxl313_data adxl313_data_##inst;                                            \
                                                                                                   \
	static const struct adxl313_config adxl313_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.range = DT_INST_PROP(inst, range),                                                \
		.odr = DT_INST_PROP(inst, odr),                                                    \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl313_init, NULL, &adxl313_data_##inst,               \
				     &adxl313_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &adxl313_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(ADXL313_DEFINE)

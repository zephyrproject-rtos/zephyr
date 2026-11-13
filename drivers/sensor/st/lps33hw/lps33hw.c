/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lps33hw

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "lps33hw.h"

LOG_MODULE_REGISTER(LPS33HW, CONFIG_SENSOR_LOG_LEVEL);

static int lps33hw_odr_to_reg(uint8_t odr, uint8_t *reg)
{
	switch (odr) {
	case 1:
		*reg = 1;
		break;
	case 10:
		*reg = 2;
		break;
	case 25:
		*reg = 3;
		break;
	case 50:
		*reg = 4;
		break;
	case 75:
		*reg = 5;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int lps33hw_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct lps33hw_config *config = dev->config;
	struct lps33hw_data *data = dev->data;
	uint8_t sample[5];
	uint32_t pressure;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PRESS &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ret = i2c_burst_read_dt(&config->i2c, LPS33HW_REG_PRESS_OUT_XL, sample, sizeof(sample));
	if (ret < 0) {
		LOG_ERR("Failed to read sensor sample (%d)", ret);
		return ret;
	}

	pressure = sys_get_le24(sample);
	data->pressure = (pressure & BIT(23)) != 0U
				 ? (int32_t)(pressure | 0xFF000000U)
				 : (int32_t)pressure;
	data->temperature = (int16_t)sys_get_le16(&sample[3]);

	return 0;
}

static int lps33hw_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct lps33hw_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_PRESS:
		/* Pressure sensitivity is 4096 LSB/hPa; sensor API uses kPa. */
		return sensor_value_from_micro(val, (int64_t)data->pressure * 100000 / 4096);
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Temperature sensitivity is 100 LSB/degree Celsius. */
		return sensor_value_from_micro(val, (int64_t)data->temperature * 10000);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, lps33hw_api) = {
	.sample_fetch = lps33hw_sample_fetch,
	.channel_get = lps33hw_channel_get,
};

static int lps33hw_reset(const struct device *dev)
{
	const struct lps33hw_config *config = dev->config;
	int64_t timeout;
	uint8_t value;
	int ret;

	ret = i2c_reg_update_byte_dt(&config->i2c, LPS33HW_REG_CTRL_REG2,
				     LPS33HW_CTRL_REG2_SWRESET, LPS33HW_CTRL_REG2_SWRESET);
	if (ret < 0) {
		return ret;
	}

	timeout = k_uptime_get() + LPS33HW_RESET_TIMEOUT_MS;
	do {
		ret = i2c_reg_read_byte_dt(&config->i2c, LPS33HW_REG_CTRL_REG2, &value);
		if (ret < 0) {
			return ret;
		}

		if ((value & LPS33HW_CTRL_REG2_SWRESET) == 0U) {
			return 0;
		}

		k_msleep(1);
	} while (k_uptime_get() < timeout);

	return -ETIMEDOUT;
}

static int lps33hw_init(const struct device *dev)
{
	const struct lps33hw_config *config = dev->config;
	uint8_t chip_id;
	uint8_t odr;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, LPS33HW_REG_WHO_AM_I, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed to read chip ID (%d)", ret);
		return ret;
	}

	if (chip_id != LPS33HW_WHO_AM_I_VALUE) {
		LOG_ERR("Invalid chip ID 0x%02x", chip_id);
		return -ENODEV;
	}

	ret = lps33hw_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset sensor (%d)", ret);
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, LPS33HW_REG_CTRL_REG2,
				     LPS33HW_CTRL_REG2_IF_ADD_INC,
				     LPS33HW_CTRL_REG2_IF_ADD_INC);
	if (ret < 0) {
		LOG_ERR("Failed to enable register auto-increment (%d)", ret);
		return ret;
	}

	ret = lps33hw_odr_to_reg(config->odr, &odr);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, LPS33HW_REG_CTRL_REG1,
				     LPS33HW_CTRL_REG1_ODR_MASK | LPS33HW_CTRL_REG1_BDU,
				     (odr << LPS33HW_CTRL_REG1_ODR_SHIFT) |
					     LPS33HW_CTRL_REG1_BDU);
	if (ret < 0) {
		LOG_ERR("Failed to configure sensor (%d)", ret);
		return ret;
	}

	return 0;
}

#define LPS33HW_DEFINE(inst)								\
	static struct lps33hw_data lps33hw_data_##inst;					\
												\
	static const struct lps33hw_config lps33hw_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
		.odr = DT_INST_PROP(inst, odr),						\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lps33hw_init, NULL,				\
				     &lps33hw_data_##inst, &lps33hw_config_##inst,	\
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
				     &lps33hw_api)

DT_INST_FOREACH_STATUS_OKAY(LPS33HW_DEFINE);

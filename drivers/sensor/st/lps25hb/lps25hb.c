/* lps25hb.c - Driver for LPS25HB pressure and temperature sensor */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lps25hb_press

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "lps25hb.h"

LOG_MODULE_REGISTER(LPS25HB, CONFIG_SENSOR_LOG_LEVEL);

static inline int lps25hb_power_ctrl(const struct device *dev, uint8_t value)
{
	const struct lps25hb_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, LPS25HB_REG_CTRL_REG1,
				      LPS25HB_MASK_CTRL_REG1_PD,
				      value << LPS25HB_SHIFT_CTRL_REG1_PD);
}

static inline int lps25hb_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lps25hb_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, LPS25HB_REG_CTRL_REG1,
				      LPS25HB_MASK_CTRL_REG1_ODR,
				      odr << LPS25HB_SHIFT_CTRL_REG1_ODR);
}

static int lps25hb_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct lps25hb_data *data = dev->data;
	const struct lps25hb_config *config = dev->config;
	uint8_t out[5];
	int offset;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	for (offset = 0; offset < sizeof(out); ++offset) {
		if (i2c_reg_read_byte_dt(&config->i2c,
					 LPS25HB_REG_PRESS_OUT_XL + offset,
					 out + offset) < 0) {
			LOG_DBG("failed to read sample");
			return -EIO;
		}
	}

	data->sample_press = (int32_t)((uint32_t)(out[0]) |
					((uint32_t)(out[1]) << 8) |
					((uint32_t)(out[2]) << 16));
	data->sample_temp = (int16_t)((uint16_t)(out[3]) |
					((uint16_t)(out[4]) << 8));

	return 0;
}

static inline void lps25hb_press_convert(struct sensor_value *val,
					 int32_t raw_val)
{
	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Also convert hPa into kPa */
	val->val1 = raw_val / 40960;

	/* For the decimal part use (3125 / 128) as a factor instead of
	 * (1000000 / 40960) to avoid int32 overflow
	 */
	val->val2 = (raw_val % 40960) * 3125 / 128;
}

static inline void lps25hb_temp_convert(struct sensor_value *val,
					int16_t raw_val)
{
	int32_t uval;

	/* val = raw_val / 480 + 42.5 */
	uval = (int32_t)raw_val * 1000000 / 480 + 42500000;
	val->val1 = (raw_val * 10 / 480 + 425) / 10;
	val->val2 = uval % 1000000;
}

static int lps25hb_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps25hb_data *data = dev->data;

	if (chan == SENSOR_CHAN_PRESS) {
		lps25hb_press_convert(val, data->sample_press);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		lps25hb_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lps25hb_api_funcs = {
	.sample_fetch = lps25hb_sample_fetch,
	.channel_get = lps25hb_channel_get,
};

static int lps25hb_init_chip(const struct device *dev)
{
	const struct lps25hb_config *config = dev->config;
	uint8_t chip_id;

	lps25hb_power_ctrl(dev, 0);
	k_busy_wait(USEC_PER_MSEC * 50U);

	if (lps25hb_power_ctrl(dev, 1) < 0) {
		LOG_DBG("failed to power on device");
		return -EIO;
	}

	k_busy_wait(USEC_PER_MSEC * 20U);

	if (i2c_reg_read_byte_dt(&config->i2c, LPS25HB_REG_WHO_AM_I,
				 &chip_id) < 0) {
		LOG_DBG("failed reading chip id");
		goto err_poweroff;
	}
	if (chip_id != LPS25HB_VAL_WHO_AM_I) {
		LOG_DBG("invalid chip id 0x%x", chip_id);
		goto err_poweroff;
	}

	LOG_DBG("chip id 0x%x", chip_id);

	if (lps25hb_set_odr_raw(dev, LPS25HB_DEFAULT_SAMPLING_RATE)
				< 0) {
		LOG_DBG("failed to set sampling rate");
		goto err_poweroff;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, LPS25HB_REG_CTRL_REG1,
				   LPS25HB_MASK_CTRL_REG1_BDU,
				   (1 << LPS25HB_SHIFT_CTRL_REG1_BDU)) < 0) {
		LOG_DBG("failed to set BDU");
		goto err_poweroff;
	}

	return 0;

err_poweroff:
	lps25hb_power_ctrl(dev, 0);
	return -EIO;
}

static int lps25hb_init(const struct device *dev)
{
	const struct lps25hb_config * const config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	if (lps25hb_init_chip(dev) < 0) {
		LOG_DBG("failed to initialize chip");
		return -EIO;
	}

	return 0;
}

#define LPS25HB_DEFINE(inst)									\
	static struct lps25hb_data lps25hb_data_##inst;						\
												\
	static const struct lps25hb_config lps25hb_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lps25hb_init, NULL,					\
			      &lps25hb_data_##inst, &lps25hb_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &lps25hb_api_funcs);			\

DT_INST_FOREACH_STATUS_OKAY(LPS25HB_DEFINE)

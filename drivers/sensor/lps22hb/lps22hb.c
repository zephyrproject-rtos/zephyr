/* lps22hb.c - Driver for LPS22HB pressure and temperature sensor */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lps22hb_press

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "lps22hb.h"

LOG_MODULE_REGISTER(LPS22HB, CONFIG_SENSOR_LOG_LEVEL);

static inline int lps22hb_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lps22hb_data *data = dev->data;
	const struct lps22hb_config *config = dev->config;

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LPS22HB_REG_CTRL_REG1,
				   LPS22HB_MASK_CTRL_REG1_ODR,
				   odr << LPS22HB_SHIFT_CTRL_REG1_ODR);
}

static int lps22hb_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct lps22hb_data *data = dev->data;
	const struct lps22hb_config *config = dev->config;
	uint8_t out[5];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (i2c_burst_read(data->i2c_master, config->i2c_slave_addr,
			   LPS22HB_REG_PRESS_OUT_XL, out, 5) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_press = (int32_t)((uint32_t)(out[0]) |
				     ((uint32_t)(out[1]) << 8) |
				     ((uint32_t)(out[2]) << 16));
	data->sample_temp = (int16_t)((uint16_t)(out[3]) |
				    ((uint16_t)(out[4]) << 8));

	return 0;
}

static inline void lps22hb_press_convert(struct sensor_value *val,
					 int32_t raw_val)
{
	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((int32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lps22hb_temp_convert(struct sensor_value *val,
					int16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = ((int32_t)raw_val % 100) * 10000;
}

static int lps22hb_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps22hb_data *data = dev->data;

	if (chan == SENSOR_CHAN_PRESS) {
		lps22hb_press_convert(val, data->sample_press);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		lps22hb_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lps22hb_api_funcs = {
	.sample_fetch = lps22hb_sample_fetch,
	.channel_get = lps22hb_channel_get,
};

static int lps22hb_init_chip(const struct device *dev)
{
	struct lps22hb_data *data = dev->data;
	const struct lps22hb_config *config = dev->config;
	uint8_t chip_id;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LPS22HB_REG_WHO_AM_I, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		goto err_poweroff;
	}

	if (chip_id != LPS22HB_VAL_WHO_AM_I) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		goto err_poweroff;
	}

	if (lps22hb_set_odr_raw(dev, LPS22HB_DEFAULT_SAMPLING_RATE) < 0) {
		LOG_DBG("Failed to set sampling rate");
		goto err_poweroff;
	}

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LPS22HB_REG_CTRL_REG1,
				LPS22HB_MASK_CTRL_REG1_BDU,
				(1 << LPS22HB_SHIFT_CTRL_REG1_BDU)) < 0) {
		LOG_DBG("Failed to set BDU");
		goto err_poweroff;
	}

	return 0;

err_poweroff:
	return -EIO;
}

static int lps22hb_init(const struct device *dev)
{
	const struct lps22hb_config * const config = dev->config;
	struct lps22hb_data *data = dev->data;

	data->i2c_master = device_get_binding(config->i2c_master_dev_name);

	if (!data->i2c_master) {
		LOG_DBG("I2c master not found: %s",
			    config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (lps22hb_init_chip(dev) < 0) {
		LOG_DBG("Failed to initialize chip");
		return -EIO;
	}

	return 0;
}

static const struct lps22hb_config lps22hb_config = {
	.i2c_master_dev_name = DT_INST_BUS_LABEL(0),
	.i2c_slave_addr = DT_INST_REG_ADDR(0),
};

static struct lps22hb_data lps22hb_data;

DEVICE_AND_API_INIT(lps22hb, DT_INST_LABEL(0), lps22hb_init,
		    &lps22hb_data, &lps22hb_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lps22hb_api_funcs);

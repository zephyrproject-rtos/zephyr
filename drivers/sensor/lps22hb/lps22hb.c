/* lps22hb.c - Driver for LPS22HB pressure and temperature sensor */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#include "lps22hb.h"

static inline int lps22hb_set_odr_raw(struct device *dev, u8_t odr)
{
	struct lps22hb_data *data = dev->driver_data;
	const struct lps22hb_config *config = dev->config->config_info;

	return i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				   LPS22HB_REG_CTRL_REG1,
				   LPS22HB_MASK_CTRL_REG1_ODR,
				   odr << LPS22HB_SHIFT_CTRL_REG1_ODR);
}

static int lps22hb_sample_fetch(struct device *dev,
				enum sensor_channel chan)
{
	struct lps22hb_data *data = dev->driver_data;
	const struct lps22hb_config *config = dev->config->config_info;
	u8_t out[5];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (i2c_burst_read(data->i2c_master, config->i2c_slave_addr,
			   LPS22HB_REG_PRESS_OUT_XL, out, 5) < 0) {
		SYS_LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_press = (s32_t)((u32_t)(out[0]) |
				     ((u32_t)(out[1]) << 8) |
				     ((u32_t)(out[2]) << 16));
	data->sample_temp = (s16_t)((u16_t)(out[3]) |
				    ((u16_t)(out[4]) << 8));

	return 0;
}

static inline void lps22hb_press_convert(struct sensor_value *val,
					 s32_t raw_val)
{
	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Convert raw_val to val in kPa */
	val->val1 = (raw_val >> 12) / 10;
	val->val2 = (raw_val >> 12) % 10 * 100000 +
		(((s32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lps22hb_temp_convert(struct sensor_value *val,
					s16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = (s32_t)raw_val % 100;
}

static int lps22hb_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps22hb_data *data = dev->driver_data;

	if (chan == SENSOR_CHAN_PRESS) {
		lps22hb_press_convert(val, data->sample_press);
	} else if (chan == SENSOR_CHAN_TEMP) {
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

static int lps22hb_init_chip(struct device *dev)
{
	struct lps22hb_data *data = dev->driver_data;
	const struct lps22hb_config *config = dev->config->config_info;
	u8_t chip_id;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      LPS22HB_REG_WHO_AM_I, &chip_id) < 0) {
		SYS_LOG_DBG("Failed reading chip id");
		goto err_poweroff;
	}

	if (chip_id != LPS22HB_VAL_WHO_AM_I) {
		SYS_LOG_DBG("Invalid chip id 0x%x", chip_id);
		goto err_poweroff;
	}

	if (lps22hb_set_odr_raw(dev, LPS22HB_DEFAULT_SAMPLING_RATE) < 0) {
		SYS_LOG_DBG("Failed to set sampling rate");
		goto err_poweroff;
	}

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
				LPS22HB_REG_CTRL_REG1,
				LPS22HB_MASK_CTRL_REG1_BDU,
				(1 << LPS22HB_SHIFT_CTRL_REG1_BDU)) < 0) {
		SYS_LOG_DBG("Failed to set BDU");
		goto err_poweroff;
	}

	return 0;

err_poweroff:
	return -EIO;
}

static int lps22hb_init(struct device *dev)
{
	const struct lps22hb_config * const config = dev->config->config_info;
	struct lps22hb_data *data = dev->driver_data;

	data->i2c_master = device_get_binding(config->i2c_master_dev_name);

	if (!data->i2c_master) {
		SYS_LOG_DBG("I2c master not found: %s",
			    config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (lps22hb_init_chip(dev) < 0) {
		SYS_LOG_DBG("Failed to initialize chip");
		return -EIO;
	}

	return 0;
}

static const struct lps22hb_config lps22hb_config = {
	.i2c_master_dev_name = CONFIG_LPS22HB_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = CONFIG_LPS22HB_I2C_ADDR,
};

static struct lps22hb_data lps22hb_data;

DEVICE_AND_API_INIT(lps22hb, CONFIG_LPS22HB_DEV_NAME, lps22hb_init,
		    &lps22hb_data, &lps22hb_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lps22hb_api_funcs);

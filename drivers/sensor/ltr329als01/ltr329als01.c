/*
 * Copyright (c) 2019 Vaishali Pathak vaishali@electronut.in
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <misc/byteorder.h>
#include <logging/log.h>
#include <i2c.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>

LOG_MODULE_REGISTER(ltr329als01);

#include "ltr329als01.h"

struct ltr329als01_data {
	struct device *i2c_dev;
	u16_t lux_val;
};

/**
 * @brief check for new data
 *
 * @return true
 * @return false
 */

static bool ALS_check_for_new_valid_data(struct device *i2c_dev)
{
	u8_t status;

	i2c_reg_read_byte(i2c_dev, ALS_ADDR, ALS_STATUS_REG, &status);

	if ((status & 0x04) && ((status & 0x80) == 0)) {
		return true;
	} else {
		return false;
	}
}

/**
 * @brief get integration time in ms from ALS_MEAS_RATE register
 *
 * @return int
 */
static int ALS_get_integration_time(struct device *i2c_dev)
{
	int integration_time_lookup[8] = {
		100, 50, 200, 400, 150, 250, 300, 350
	};

	u8_t meas_rate_reg_data;

	if (i2c_reg_read_byte(i2c_dev, ALS_ADDR, ALS_MEAS_RATE_REG,
		&meas_rate_reg_data) != 0) {
		LOG_ERR("get ch err");
	}

	u8_t integration_time_byte = (meas_rate_reg_data & 0x38) >> 3;

	int als_integration_time_ms =
		integration_time_lookup[integration_time_byte];

	return als_integration_time_ms;
}

/**
 * @brief get adc values of channel-0 and channel-1 from als data registers
 *
 * @return u16_t*
 */
static u16_t *ALS_get_channels_data(struct device *i2c_dev)
{
	u8_t als_buffer[4];
	static u16_t als_channels_data[2];

	for (int i = 0; i < 4; i++) {
		if (i2c_reg_read_byte(i2c_dev, ALS_ADDR, ALS_DATA_CH1_0_REG + i,
			      &als_buffer[i]) != 0) {
			LOG_ERR("get ch err");
		}
	}

	als_channels_data[0] = (als_buffer[1] << 8) | als_buffer[0];
	als_channels_data[1] = (als_buffer[3] << 8) | als_buffer[2];

	LOG_DBG("channels %d %d", als_channels_data[0], als_channels_data[1]);

	return als_channels_data;
}

/**
 * @brief get gain values from ALS_CONTR register
 *
 * @return int
 */
static int ALS_get_gain(struct device *i2c_dev)
{
	int gain_lookup[8] = { 1, 2, 4, 8, RESERVED, RESERVED, 48, 96 };
	u8_t contr_reg_data;

	i2c_reg_read_byte(i2c_dev, ALS_ADDR, ALS_CONTR_REG, &contr_reg_data);

	u8_t als_gain_byte = (contr_reg_data & 0x1C) >> 2;

	int als_gain = gain_lookup[als_gain_byte];

	return als_gain;
}

/**
 * @brief calculate lux values from lux buffer
 *
 * @return float
 */
static float ALS_get_lux(struct device *i2c_dev)
{
	u16_t *als_channels_data;

	als_channels_data = ALS_get_channels_data(i2c_dev);

	float ratio = (float)als_channels_data[1] /
		      (float)(als_channels_data[0] + als_channels_data[1]);

	int ALS_GAIN = ALS_get_gain(i2c_dev);

	float ALS_INT = (float)ALS_get_integration_time(i2c_dev) / 100;

	float als_lux;

	if (ratio < 0.45) {
		als_lux = ((1.7743 * (float)als_channels_data[0]) +
			   (1.1059 * (float)als_channels_data[1])) /
			  (ALS_GAIN * ALS_INT);
	} else if (ratio < 0.64 && ratio >= 0.45) {
		als_lux = ((4.2785 * (float)als_channels_data[0]) -
			   (1.9548 * (float)als_channels_data[1])) /
			  (ALS_GAIN * ALS_INT);
	} else if (ratio < 0.85 && ratio >= 0.64) {
		als_lux = ((0.5926 * (float)als_channels_data[0]) +
			   (0.1185 * (float)als_channels_data[1])) /
			  (ALS_GAIN * ALS_INT);
	} else {
		als_lux = 0;
	}
	return als_lux;
}

/**
 * @brief fetch a sample from the sensor
 *
 * @return 0 for success and EAGAIN for fail
 */
static int ltr329als01_sample_fetch(struct device *dev,
				    enum sensor_channel chan)
{
	struct ltr329als01_data *ltr_data = dev->driver_data;

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT) {
		if (ALS_check_for_new_valid_data(ltr_data->i2c_dev) == true) {

			float als_val = ALS_get_lux(ltr_data->i2c_dev);

			ltr_data->lux_val = als_val;
			return 0;
		} else {
			return -EAGAIN;
		}

	} else {
		return -ENOTSUP;
	}
}

/**
 * @brief sensor value get
 *
 * @return 0 for success and EAGAIN for fail
 */
static int ltr329als01_channel_get(struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct ltr329als01_data *ltr_data = dev->driver_data;

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT) {

		float als_val = ltr_data->lux_val;

		val->val1 = (s32_t)als_val;
		val->val2 = ((s32_t)(als_val * 1000000)) % 1000000;

		return 0;

	} else {
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api ltr329als01_api = {
	.sample_fetch = &ltr329als01_sample_fetch,
	.channel_get = &ltr329als01_channel_get,
};

static int ltr329als01_init(struct device *dev)
{
	struct ltr329als01_data *drv_data = dev->driver_data;
	u8_t manf_id = 0;

	k_sleep(100); /* wait after power on */

	LOG_INF("ltr329als01 initialised...\r\n");

	drv_data->i2c_dev = device_get_binding("I2C_0");
	if (!drv_data->i2c_dev) {
		LOG_ERR("i2c master not found.");
		return -EINVAL;
	}

	if (i2c_reg_read_byte(drv_data->i2c_dev, ALS_ADDR, MANUFAC_ID_REG,
		&manf_id) == 0) {
		LOG_DBG("found! %x \r\n", manf_id);
	} else {
		return -EIO;
	}

	/* set measurement and intergration time */
	u8_t mea_int_time = 0x1b;

	if (i2c_reg_write_byte(drv_data->i2c_dev, ALS_ADDR, ALS_MEAS_RATE_REG,
		mea_int_time) != 0) {
		LOG_ERR("measurement and intergration time not set!");
	}

/* enable ltr sensor - set to active mode  For Int time = 400ms, Meas
 *  rate = 500ms, Command = 0x1B
 */
	u8_t enable_ltr = 0x01;

	if (i2c_reg_write_byte(drv_data->i2c_dev, ALS_ADDR, ALS_CONTR_REG,
		enable_ltr) != 0) {
		LOG_ERR("not active");
	}

	k_sleep(10); /* wait after active mode set */

	return 0;
}

static struct ltr329als01_data ltr_data;

DEVICE_AND_API_INIT(ltr329als01, "LTR_0", ltr329als01_init, &ltr_data, NULL,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ltr329als01_api);

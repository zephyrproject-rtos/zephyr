/*
 * Copyright (c) 2019 Electronut Labs
 * Copyright (c) 2023 Trent Piepho <tpiepho@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include "si7006.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(si7006, CONFIG_SENSOR_LOG_LEVEL);

struct si7006_data {
	uint16_t temperature;
	uint16_t humidity;
};

struct si7006_config {
	struct i2c_dt_spec i2c;
	/** Use "read temp" vs "read old temp" command, the latter only with SiLabs sensors. */
	uint8_t read_temp_cmd;
};

/**
 * @brief function to get relative humidity
 *
 * @return int 0 on success
 */
static int si7006_get_humidity(const struct device *dev)
{
	struct si7006_data *si_data = dev->data;
	const struct si7006_config *config = dev->config;
	int retval;
	uint8_t hum[2];

	retval = i2c_burst_read_dt(&config->i2c,
				   SI7006_MEAS_REL_HUMIDITY_MASTER_MODE, hum,
				   sizeof(hum));

	if (retval == 0) {
		si_data->humidity = (hum[0] << 8) | hum[1];
	} else {
		LOG_ERR("read register err");
	}

	return retval;
}

/**
 * @brief function to get temperature
 *
 * Note that for Si7006 type sensors, si7006_get_humidity must be called before
 * calling si7006_get_temperature, as the get old temperature command is used.
 *
 * @return int 0 on success
 */

static int si7006_get_temperature(const struct device *dev)
{
	struct si7006_data *si_data = dev->data;
	const struct si7006_config *config = dev->config;
	uint8_t temp[2];
	int retval;

	retval = i2c_burst_read_dt(&config->i2c, config->read_temp_cmd, temp,
				   sizeof(temp));

	if (retval == 0) {
		si_data->temperature = (temp[0] << 8) | temp[1];
	} else {
		LOG_ERR("read register err");
	}

	return retval;
}

/**
 * @brief fetch a sample from the sensor
 *
 * @return 0
 */
static int si7006_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	int retval;

	retval = si7006_get_humidity(dev);
	if (retval == 0) {
		retval = si7006_get_temperature(dev);
	}

	return retval;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int si7006_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct si7006_data *si_data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {

		int32_t temp_ucelcius = (((17572 * (int32_t)si_data->temperature)
					/ 65536) - 4685) * 10000;

		val->val1 = temp_ucelcius / 1000000;
		val->val2 = temp_ucelcius % 1000000;

		LOG_DBG("temperature = val1:%d, val2:%d", val->val1, val->val2);

		return 0;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {

		int32_t relative_humidity = (((125 * (int32_t)si_data->humidity)
					    / 65536) - 6) * 1000000;

		val->val1 = relative_humidity / 1000000;
		val->val2 = relative_humidity % 1000000;

		LOG_DBG("humidity = val1:%d, val2:%d", val->val1, val->val2);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api si7006_api = {
	.sample_fetch = &si7006_sample_fetch,
	.channel_get = &si7006_channel_get,
};

/**
 * @brief initialize the sensor
 *
 * @return 0 for success
 */

static int si7006_init(const struct device *dev)
{
	const struct si7006_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	LOG_DBG("si7006 init ok");

	return 0;
}

#define SI7006_DEFINE(inst, temp_cmd)							\
	static struct si7006_data si7006_data_##inst;					\
											\
	static const struct si7006_config si7006_config_##inst = {			\
		.i2c = I2C_DT_SPEC_GET(inst),						\
		.read_temp_cmd = temp_cmd,						\
	};										\
											\
	SENSOR_DEVICE_DT_DEFINE(inst, si7006_init, NULL,				\
				&si7006_data_##inst, &si7006_config_##inst,		\
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7006_api); \

DT_FOREACH_STATUS_OKAY_VARGS(silabs_si7006, SI7006_DEFINE, SI7006_READ_OLD_TEMP);
DT_FOREACH_STATUS_OKAY_VARGS(sensirion_sht21, SI7006_DEFINE, SI7006_MEAS_TEMP_MASTER_MODE);

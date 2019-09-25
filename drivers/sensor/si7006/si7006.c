/*
 * Copyright (c) 2019 Electronut Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <i2c.h>
#include <logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include "si7006.h"

LOG_MODULE_REGISTER(si7006, CONFIG_SENSOR_LOG_LEVEL);

struct si7006_data {
	struct device *i2c_dev;
	u16_t temperature;
	u16_t humidity;
};

/**
 * @brief function to get relative humidity
 *
 * @return int 0 on success
 */
static int si7006_get_humidity(struct device *i2c_dev,
			       struct si7006_data *si_data)
{
	int retval;
	u8_t hum[2];

	retval = i2c_burst_read(i2c_dev, DT_INST_0_SILABS_SI7006_BASE_ADDRESS,
		SI7006_MEAS_REL_HUMIDITY_MASTER_MODE, hum, sizeof(hum));

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
 * @return int 0 on success
 */

static int si7006_get_temperature(struct device *i2c_dev,
				  struct si7006_data *si_data)
{
	u8_t temp[2];
	int retval;

	retval = i2c_burst_read(i2c_dev, DT_INST_0_SILABS_SI7006_BASE_ADDRESS,
		SI7006_MEAS_TEMP_MASTER_MODE, temp, sizeof(temp));

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
static int si7006_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	int retval;
	struct si7006_data *si_data = dev->driver_data;

	retval = si7006_get_temperature(si_data->i2c_dev, si_data);
	if (retval == 0) {
		retval = si7006_get_humidity(si_data->i2c_dev, si_data);
	}

	return retval;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int si7006_channel_get(struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct si7006_data *si_data = dev->driver_data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {

		s32_t temp_ucelcius = ((17572 * (s32_t)si_data->temperature)
				       / 65536) * 10000;

		val->val1 = temp_ucelcius / 1000000;
		val->val2 = temp_ucelcius % 1000000;

		LOG_DBG("temperature = val1:%d, val2:%d", val->val1, val->val2);

		return 0;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {

		s32_t relative_humidity = (((125 * (s32_t)si_data->humidity)
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
 * @brief initiasize the sensor
 *
 * @return 0 for success
 */

static int si7006_init(struct device *dev)
{
	struct si7006_data *drv_data = dev->driver_data;

	drv_data->i2c_dev = device_get_binding(
		DT_INST_0_SILABS_SI7006_BUS_NAME);

	if (!drv_data->i2c_dev) {
		LOG_ERR("i2c master not found.");
		return -EINVAL;
	}

	LOG_DBG("si7006 init ok");

	return 0;
}

static struct si7006_data si_data;

DEVICE_AND_API_INIT(si7006, DT_INST_0_SILABS_SI7006_LABEL, si7006_init,
	&si_data, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7006_api);

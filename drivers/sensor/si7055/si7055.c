/*
 * Copyright (c) 2020 Laird Connectivity
 * Copyright (c) 2019 Electronut Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si7055

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include "si7055.h"
#if CONFIG_SI7055_ENABLE_CHECKSUM
#include <zephyr/sys/crc.h>
#endif

LOG_MODULE_REGISTER(si7055, CONFIG_SENSOR_LOG_LEVEL);

struct si7055_data {
	uint16_t temperature;
};

struct si7055_config {
	struct i2c_dt_spec i2c;
};

/**
 * @brief function to get temperature
 *
 * @return int 0 on success
 *         -EIO for I/O and checksum errors
 */
static int si7055_get_temperature(const struct device *dev)
{
	struct si7055_data *si_data = dev->data;
	const struct si7055_config *config = dev->config;
	int retval;
	#if CONFIG_SI7055_ENABLE_CHECKSUM
	uint8_t temp[SI7055_TEMPERATURE_READ_WITH_CHECKSUM_SIZE];
	#else
	uint8_t temp[SI7055_TEMPERATURE_READ_NO_CHECKSUM_SIZE];
	#endif

	retval = i2c_burst_read_dt(&config->i2c, SI7055_MEAS_TEMP_MASTER_MODE,
				   temp, sizeof(temp));

	/* Refer to
	 * https://www.silabs.com/documents/public/data-sheets/Si7050-1-3-4-5-A20.pdf
	 */

	if (retval == 0) {
		#if CONFIG_SI7055_ENABLE_CHECKSUM
		if (crc8(temp, SI7055_DATA_SIZE, SI7055_CRC_POLY,
			SI7055_CRC_INIT, false) != temp[SI7055_DATA_SIZE]){
			LOG_ERR("checksum failed.\n");
			return(-EIO);
		}
		#endif
		si_data->temperature = (temp[SI7055_TEMPERATURE_DATA_BYTE_0]
					<< 8) |
					temp[SI7055_TEMPERATURE_DATA_BYTE_1];
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
static int si7055_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	int retval;

	retval = si7055_get_temperature(dev);

	return retval;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int si7055_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct si7055_data *si_data = dev->data;

	/* Refer to
	 * https://www.silabs.com/documents/public/data-sheets/Si7050-1-3-4-5-A20.pdf
	 */

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {

		int32_t temp_ucelcius = (((SI7055_CONV_FACTOR_1 *
					(int32_t)si_data->temperature) /
					(__UINT16_MAX__ + 1)) -
					SI7055_CONV_FACTOR_2) *
					SI7055_MULTIPLIER;

		val->val1 = temp_ucelcius / SI7055_DIVIDER;
		val->val2 = temp_ucelcius % SI7055_DIVIDER;

		LOG_DBG("temperature = val1:%d, val2:%d",
			val->val1, val->val2);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api si7055_api = {
	.sample_fetch = &si7055_sample_fetch,
	.channel_get = &si7055_channel_get,
};

/**
 * @brief initialize the sensor
 *
 * @return 0 for success
 */

static int si7055_init(const struct device *dev)
{
	const struct si7055_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	LOG_DBG("si7055 init ok");

	return 0;
}

static struct si7055_data si_data;

static const struct si7055_config si7055_config_inst = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
};

DEVICE_DT_INST_DEFINE(0, si7055_init, NULL, &si_data, &si7055_config_inst,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7055_api);

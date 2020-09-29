/*
 * Copyright (c) 2020 Laird Connectivity
 * Copyright (c) 2019 Electronut Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si7055

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <logging/log.h>
#include <drivers/i2c.h>
#include "si7055.h"
#if CONFIG_SI7055_ENABLE_CHECKSUM
#include <sys/crc.h>
#endif

LOG_MODULE_REGISTER(si7055, CONFIG_SENSOR_LOG_LEVEL);

struct si7055_data {
	const struct device *i2c_dev;
	uint16_t temperature;
};

/**
 * @brief function to get temperature
 *
 * @return int 0 on success
 *         -EIO for I/O and checksum errors
 */
static int si7055_get_temperature(const struct device *i2c_dev,
				  struct si7055_data *si_data)
{
	int retval;
	#if CONFIG_SI7055_ENABLE_CHECKSUM
	uint8_t temp[SI7055_TEMPERATURE_READ_WITH_CHECKSUM_SIZE];
	#else
	uint8_t temp[SI7055_TEMPERATURE_READ_NO_CHECKSUM_SIZE];
	#endif

	retval = i2c_burst_read(i2c_dev, DT_INST_REG_ADDR(0),
				SI7055_MEAS_TEMP_MASTER_MODE,
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
	struct si7055_data *si_data = dev->data;

	retval = si7055_get_temperature(si_data->i2c_dev, si_data);

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
	struct si7055_data *drv_data = dev->data;

	drv_data->i2c_dev = device_get_binding(
		DT_INST_BUS_LABEL(0));

	if (!drv_data->i2c_dev) {
		LOG_ERR("i2c master not found.");
		return -EINVAL;
	}

	LOG_DBG("si7055 init ok");

	return 0;
}

static struct si7055_data si_data;

DEVICE_AND_API_INIT(si7055, DT_INST_LABEL(0), si7055_init,
	&si_data, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7055_api);

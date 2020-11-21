/*
 * Copyright (c) 2020 Nikolaus Huber, Uppsala University
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si7021

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <logging/log.h>
#include <drivers/i2c.h>
#if CONFIG_SI7021_ENABLE_CHECKSUM
#include <sys/crc.h>
#endif

#include "si7021.h"

LOG_MODULE_REGISTER(si7021, CONFIG_SENSOR_LOG_LEVEL);

struct si7021_data {
	const struct device *i2c_dev;
	uint16_t rh_code;
	uint16_t temp_code;
};

/**
 * @brief Function to get both the relative humidity and temperature
 *
 * @return int 0 on success, -EIO on either I/O or checksum error
 */
static int si7021_get_rh_and_temp(struct si7021_data *si_data)
{
	int retval;

    #if CONFIG_SI7021_ENABLE_CHECKSUM
	uint8_t buffer[SI7021_READ_WITH_CHECKSUM_SIZE];
    #else
	uint8_t buffer[SI7021_READ_NO_CHECKSUM_SIZE];
    #endif
	uint8_t command = SI7021_MEAS_RH_HOLD_MASTER;

	retval = i2c_write_read(si_data->i2c_dev, DT_INST_REG_ADDR(0), &command, 1, buffer, sizeof(buffer));

	if (retval != 0) {
		LOG_ERR("Error while reading sensor data.");
		return -EIO;
	}

    #if CONFIG_SI7021_ENABLE_CHECKSUM
	if (crc8(buffer, SI7021_DATA_SIZE, SI7021_CRC_POLY, SI7021_CRC_INIT, false) != buffer[2]) {
		LOG_ERR("Error with checksum.");
		return -EIO;
	}
    #endif

	si_data->rh_code = buffer[0] << 8 | buffer[1];

	command = SI7021_MEAS_TEMP_HOLD_MASTER;

    /* We can reuse the same buffer */
	retval = i2c_write_read(si_data->i2c_dev, DT_INST_REG_ADDR(0), &command, 1, buffer, sizeof(buffer));

	if (retval != 0) {
		LOG_ERR("Error while reading sensor data.");
		return -EIO;
	}

    #if CONFIG_SI7021_ENABLE_CHECKSUM
	if (crc8(buffer, SI7021_DATA_SIZE, SI7021_CRC_POLY, SI7021_CRC_INIT, false) != buffer[2]) {
		LOG_ERR("Error with checksum.");
		return -EIO;
	}
    #endif

	si_data->temp_code = buffer[0] << 8 | buffer[1];

	return 0;
}

/**
 * @brief Fatch a sample from the sensor
 *
 * @return int 0 on success, -EIO if an error occurs
 */
static int si7021_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int retval;

	struct si7021_data *si_data = dev->data;

	retval = si7021_get_rh_and_temp(si_data);
	return retval;
}

/**
 * @brief Get sensor value according on requested channel
 *
 * @ return -ENOTSUP if requesting unsupported channel
 */
static int si7021_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	struct si7021_data *si_data = dev->data;

	/* Refer to data sheet for formulas below */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		int32_t temp_celsius = ((17572 * si_data->temp_code) / (65536 + 1) - 4685) * 10000;

		val->val1 = temp_celsius / 1000000;
		val->val2 = temp_celsius % 1000000;

		LOG_DBG("Temp = val1:%d, val2:%d", val->val1, val->val2);
		return 0;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		int32_t rh_proc = ((12500 * si_data->rh_code) / (65536 + 1) - 600) * 10000;

		val->val1 = rh_proc / 1000000;
		val->val2 = rh_proc % 1000000;

		LOG_DBG("Humidity = val1:%d, val2:%d", val->val1, val->val2);
		return 0;
	} else   {
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api si7021_api = {
	.sample_fetch = &si7021_sample_fetch,
	.channel_get = &si7021_channel_get,
};

/**
 * @brief Initialisation of the sensor
 *
 * @return int 0 if successful, -EINVAL otherwise
 */
static int si7021_init(const struct device *dev)
{
	struct si7021_data *drv_data = dev->data;

	drv_data->i2c_dev = device_get_binding(DT_INST_BUS_LABEL(0));

	if (!drv_data->i2c_dev) {
		LOG_ERR("Could not initiliase i2c.");
		return -EINVAL;
	}

	LOG_DBG("Si7021 initialisation ok.");

	return 0;
}

static struct si7021_data si_data;

DEVICE_AND_API_INIT(si7021, DT_INST_LABEL(0), si7021_init,
		&si_data, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7021_api);

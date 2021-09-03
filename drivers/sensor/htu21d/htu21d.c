/*
 * Copyright (c) 2021, Jonas Norling
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TE Connectivity / MEAS HTU21D temperature and humidity sensor.
 * The sensor is connected by I2C, address 0x40.
 *
 * The default resolution is used for RH (12 bits) and T (14 bits) samples.
 *
 * Resolution at different settings:
 *           temp °C     relative humidity %
 * 14 bits:  0.011
 * 13 bits:  0.021
 * 12 bits:  0.043       0.03
 * 11 bits:  0.085       0.06
 * 10 bits:              0.12
 *  8 bits:              0.49
 */

#define DT_DRV_COMPAT meas_htu21d

#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <sys/crc.h>
#include <logging/log.h>

#include "htu21d.h"

LOG_MODULE_REGISTER(HTU21D, CONFIG_SENSOR_LOG_LEVEL);

static int htu21d_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct htu21d_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* Check, then clear, diagnostic status bits */
		if ((data->t_sample & 0x2) != 0x0) {
			return -EIO;
		}
		data->t_sample &= 0xfffc;
		/* T = -46.85 + 175.72 * sample / 2^16
		 * Work in fixed-point 2 decimals
		 */
		const int sample = data->t_sample & 0xfffc;
		const int t = ((17572 * sample) >> 16) - 4685;

		val->val1 = t / 100;
		val->val2 = (t % 100) * 10000;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		/* Check, then clear, diagnostic status bits */
		if ((data->rh_sample & 0x2) != 0x2) {
			return -EIO;
		}
		data->rh_sample &= 0xfffc;
		/* RH% = -6 + 125 * sample / 2^16
		 * Work in fixed-point 2 decimals
		 */
		const int sample = data->rh_sample & 0xfffc;
		const int rh = ((12500 * sample) >> 16) - 600;

		val->val1 = rh / 100;
		val->val2 = (rh % 100) * 10000;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int htu21d_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct htu21d_data *data = dev->data;
	const struct htu21d_config *config = dev->config;
	int res;
	const uint8_t cmd_t[1] = { HTU21D_TRIGGER_T_NHM };
	const uint8_t cmd_rh[1] = { HTU21D_TRIGGER_RH_NHM };
	uint8_t buffer[3];
	uint8_t crc;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* Reset samples so we don't read old values if this function fails */
	data->t_sample = 0;
	data->rh_sample = 0;

	res = i2c_write_dt(&config->i2c, cmd_t, sizeof(cmd_t));
	if (res < 0) {
		LOG_ERR("Failed to trigger sample: %d", res);
		return -EIO;
	}

	/* Wait max 50 ms for 14-bit sample */
	k_sleep(K_MSEC(50));
	res = i2c_read_dt(&config->i2c, buffer, sizeof(buffer));
	crc = crc8(buffer, 2, 0x31, 0, false);
	if (buffer[2] != crc) {
		LOG_ERR("Checksum error");
		return -EIO;
	}
	data->t_sample = (buffer[0] << 8) | buffer[1];

	res = i2c_write_dt(&config->i2c, cmd_rh, sizeof(cmd_rh));
	if (res < 0) {
		LOG_ERR("Failed to trigger sample: %d", res);
		return -EIO;
	}

	/* Wait max 13 ms for 12-bit sample */
	k_sleep(K_MSEC(13));
	res = i2c_read_dt(&config->i2c, buffer, sizeof(buffer));
	crc = crc8(buffer, 2, 0x31, 0, false);
	if (buffer[2] != crc) {
		LOG_ERR("Checksum error");
		return -EIO;
	}
	data->rh_sample = (buffer[0] << 8) | buffer[1];

	return 0;
}

static const struct sensor_driver_api htu21d_driver_api = {
	.sample_fetch = htu21d_sample_fetch,
	.channel_get = htu21d_channel_get,
};

int htu21d_init(const struct device *dev)
{
	const struct htu21d_config *config = dev->config;
	struct htu21d_data *data = dev->data;
	int res;
	const uint8_t reset_command[1] = { HTU21D_SOFT_RESET };

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus %s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	/* Soft-reset the sensor */
	res = i2c_write_dt(&config->i2c, reset_command, sizeof(reset_command));
	if (res < 0) {
		LOG_ERR("Failed to write I2C");
		return -EIO;
	}

	/* Wait for reset */
	k_sleep(K_MSEC(15));

	return 0;
}

#define HTU21D_DRIVER_INIT(inst)				    \
	static struct htu21d_data drv_data_##inst;		    \
	static const struct htu21d_config drv_config_##inst = {	    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),		    \
	};							    \
	DEVICE_DT_INST_DEFINE(inst,				    \
			      &htu21d_init,			    \
			      NULL,				    \
			      &drv_data_##inst,			    \
			      &drv_config_##inst,		    \
			      POST_KERNEL,			    \
			      CONFIG_SENSOR_INIT_PRIORITY,	    \
			      &htu21d_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HTU21D_DRIVER_INIT)

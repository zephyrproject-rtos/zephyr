/*
 * Copyright (c) 2024 Amarula Solutions B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sht2x

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include "sht2x.h"

LOG_MODULE_REGISTER(SHT2X, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t sht2x_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	return crc8(buf, 2, SHT2X_CRC_POLY, SHT2X_CRC_INIT, false);
}

static int sht2x_write_command(const struct device *dev, uint8_t cmd)
{
	const struct sht2x_config *cfg = dev->config;
	uint8_t tx_buf[1] = { cmd };

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int sht2x_read_sample(const struct device *dev,
		uint16_t *sample)
{
	const struct sht2x_config *cfg = dev->config;
	uint8_t rx_buf[3];
	int rc;

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	*sample = sys_get_be16(rx_buf);
	if (sht2x_compute_crc(*sample) != rx_buf[2]) {
		LOG_ERR("Invalid CRC");
		return -EIO;
	}

	return 0;
}

static int sht2x_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct sht2x_data *data = dev->data;
	int rc;

	if (chan != SENSOR_CHAN_ALL &&
		chan != SENSOR_CHAN_AMBIENT_TEMP &&
		chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	rc = sht2x_write_command(dev, measure_cmd[READ_TEMP]);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	k_sleep(K_MSEC(measure_wait_ms[READ_TEMP]));

	rc = sht2x_read_sample(dev, &data->t_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	rc = sht2x_write_command(dev, measure_cmd[READ_HUMIDITY]);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	k_sleep(K_MSEC(measure_wait_ms[READ_HUMIDITY]));

	rc = sht2x_read_sample(dev, &data->rh_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	return 0;
}

static int sht2x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct sht2x_data *data = dev->data;

	/*
	 * See datasheet "Conversion of Signal Output" section
	 * for more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		int64_t tmp;

		tmp = data->t_sample * 175;
		val->val1 = (int32_t)(tmp / 0xFFFF) - 45;
		val->val2 = ((tmp % 0xFFFF) * 1000000) / 0xFFFF;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		uint64_t tmp;

		tmp = data->rh_sample * 125U;
		val->val1 = (uint32_t)(tmp / 0xFFFF) - 6U;
		val->val2 = (tmp % 0xFFFF) * 15625U / 1024U;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int sht2x_init(const struct device *dev)
{
	const struct sht2x_config *cfg = dev->config;
	int rc = 0;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	rc = sht2x_write_command(dev, SHT2X_CMD_RESET);
	if (rc < 0) {
		LOG_ERR("Failed to reset the device.");
		return rc;
	}

	k_sleep(K_MSEC(SHT2X_RESET_WAIT_MS));

	return 0;
}


static const struct sensor_driver_api sht2x_api = {
	.sample_fetch = sht2x_sample_fetch,
	.channel_get = sht2x_channel_get,
};

#define SHT2X_INIT(n)						\
	static struct sht2x_data sht2x_data_##n;		\
								\
	static const struct sht2x_config sht2x_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
	};							\
								\
	SENSOR_DEVICE_DT_INST_DEFINE(n,				\
			      sht2x_init,			\
			      NULL,				\
			      &sht2x_data_##n,			\
			      &sht2x_config_##n,		\
			      POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY,	\
			      &sht2x_api);

DT_INST_FOREACH_STATUS_OKAY(SHT2X_INIT)

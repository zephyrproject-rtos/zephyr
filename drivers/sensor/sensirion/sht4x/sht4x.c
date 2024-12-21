/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sht4x

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include <zephyr/drivers/sensor/sht4x.h>
#include "sht4x.h"

LOG_MODULE_REGISTER(SHT4X, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t sht4x_compute_crc(uint16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);

	return crc8(buf, 2, SHT4X_CRC_POLY, SHT4X_CRC_INIT, false);
}

static int sht4x_write_command(const struct device *dev, uint8_t cmd)
{
	const struct sht4x_config *cfg = dev->config;
	uint8_t tx_buf[1] = { cmd };

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int sht4x_read_sample(const struct device *dev,
		uint16_t *t_sample,
		uint16_t *rh_sample)
{
	const struct sht4x_config *cfg = dev->config;
	uint8_t rx_buf[6];
	int rc;

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	*t_sample = sys_get_be16(rx_buf);
	if (sht4x_compute_crc(*t_sample) != rx_buf[2]) {
		LOG_ERR("Invalid CRC for T.");
		return -EIO;
	}

	*rh_sample = sys_get_be16(&rx_buf[3]);
	if (sht4x_compute_crc(*rh_sample) != rx_buf[5]) {
		LOG_ERR("Invalid CRC for RH.");
		return -EIO;
	}

	return 0;
}

/* public API for handling the heater */
int sht4x_fetch_with_heater(const struct device *dev)
{
	struct sht4x_data *data = dev->data;
	int rc;

	rc = sht4x_write_command(dev,
			heater_cmd[data->heater_power][data->heater_duration]);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	k_sleep(K_MSEC(heater_wait_ms[data->heater_duration]));

	rc = sht4x_read_sample(dev, &data->t_sample, &data->rh_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	return 0;
}

static int sht4x_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	const struct sht4x_config *cfg = dev->config;
	struct sht4x_data *data = dev->data;
	int rc;

	if (chan != SENSOR_CHAN_ALL &&
		chan != SENSOR_CHAN_AMBIENT_TEMP &&
		chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	rc = sht4x_write_command(dev, measure_cmd[cfg->repeatability]);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	k_sleep(K_USEC(measure_wait_us[cfg->repeatability]));

	rc = sht4x_read_sample(dev, &data->t_sample, &data->rh_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	return 0;
}

static int sht4x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct sht4x_data *data = dev->data;

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

static int sht4x_attr_set(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct sht4x_data *data = dev->data;

	if (val->val1 < 0) {
		return -EINVAL;
	}

	switch ((enum sensor_attribute_sht4x)attr) {
	case SENSOR_ATTR_SHT4X_HEATER_POWER:
		if (val->val1 > SHT4X_HEATER_POWER_IDX_MAX) {
			return -EINVAL;
		}
		data->heater_power = val->val1;
		break;
	case SENSOR_ATTR_SHT4X_HEATER_DURATION:
		if (val->val1 > SHT4X_HEATER_DURATION_IDX_MAX) {
			return -EINVAL;
		}
		data->heater_duration = val->val1;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int sht4x_init(const struct device *dev)
{
	const struct sht4x_config *cfg = dev->config;
	int rc = 0;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	rc = sht4x_write_command(dev, SHT4X_CMD_RESET);
	if (rc < 0) {
		LOG_ERR("Failed to reset the device.");
		return rc;
	}

	k_sleep(K_MSEC(SHT4X_RESET_WAIT_MS));

	return 0;
}


static DEVICE_API(sensor, sht4x_api) = {
	.sample_fetch = sht4x_sample_fetch,
	.channel_get = sht4x_channel_get,
	.attr_set = sht4x_attr_set,
};

#define SHT4X_INIT(n)						\
	static struct sht4x_data sht4x_data_##n;		\
								\
	static const struct sht4x_config sht4x_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
		.repeatability = DT_INST_PROP(n, repeatability)	\
	};							\
								\
	SENSOR_DEVICE_DT_INST_DEFINE(n,				\
			      sht4x_init,			\
			      NULL,				\
			      &sht4x_data_##n,			\
			      &sht4x_config_##n,		\
			      POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY,	\
			      &sht4x_api);

DT_INST_FOREACH_STATUS_OKAY(SHT4X_INIT)

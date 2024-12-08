/*
 * Copyright (c) 2023 Ian Morris
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_hs300x

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define HS300X_STATUS_MASK (BIT(0) | BIT(1))

LOG_MODULE_REGISTER(HS300X, CONFIG_SENSOR_LOG_LEVEL);

struct hs300x_config {
	struct i2c_dt_spec bus;
};

struct hs300x_data {
	int16_t t_sample;
	uint16_t rh_sample;
};

static int hs300x_read_sample(const struct device *dev, uint16_t *t_sample, uint16_t *rh_sample)
{
	const struct hs300x_config *cfg = dev->config;
	uint8_t rx_buf[4];
	int rc;

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	if ((rx_buf[3] & HS300X_STATUS_MASK) != 0) {
		LOG_ERR("Stale data");
		return -EIO;
	}

	*rh_sample = sys_get_be16(rx_buf);
	*t_sample = sys_get_be16(&rx_buf[2]);

	/* Remove status bits (only present in temperature value)*/
	*t_sample >>= 2;

	return 0;
}

static int hs300x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct hs300x_data *data = dev->data;
	const struct hs300x_config *cfg = dev->config;
	int rc;
	uint8_t df_dummy = 0x0;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	/*
	 * By default, the sensor should be factory-programmed to operate in Sleep Mode.
	 * A Measurement Request (MR) command is required to exit the sensor
	 * from its sleep state. An MR command should consist of the 7-bit address followed
	 * by an eighth bit set to 0 (write). However, many I2C controllers cannot generate
	 * merely the address byte with no data. To overcome this limitation the MR command
	 * should be followed by a dummy byte (zero value).
	 */
	rc = i2c_write_dt(&cfg->bus, (const uint8_t *)&df_dummy, 1);
	if (rc < 0) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	/*
	 * According to datasheet maximum time to make temperature and humidity
	 * measurements is 33ms, add a little safety margin...
	 */
	k_msleep(50);

	rc = hs300x_read_sample(dev, &data->t_sample, &data->rh_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	return 0;
}

static void hs300x_temp_convert(struct sensor_value *val, int16_t raw)
{
	int32_t micro_c;

	/*
	 * Convert to micro Celsius. See datasheet "Calculating Humidity and
	 * Temperature Output" section for more details on processing sample data.
	 */
	micro_c = (((int64_t)raw * 165000000) / 16383) - 40000000;

	val->val1 = micro_c / 1000000;
	val->val2 = micro_c % 1000000;
}

static void hs300x_rh_convert(struct sensor_value *val, uint16_t raw)
{
	int32_t micro_rh;

	/*
	 * Convert to micro %RH. See datasheet "Calculating Humidity and
	 * Temperature Output" section for more details on processing sample data.
	 */
	micro_rh = ((uint64_t)raw * 100000000) / 16383;

	val->val1 = micro_rh / 1000000;
	val->val2 = micro_rh % 1000000;
}

static int hs300x_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct hs300x_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		hs300x_temp_convert(val, data->t_sample);
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		hs300x_rh_convert(val, data->rh_sample);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int hs300x_init(const struct device *dev)
{
	const struct hs300x_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(sensor, hs300x_driver_api) = {
	.sample_fetch = hs300x_sample_fetch,
	.channel_get = hs300x_channel_get,
};

#define DEFINE_HS300X(n)                                                                           \
	static struct hs300x_data hs300x_data_##n;                                                 \
                                                                                                   \
	static const struct hs300x_config hs300x_config_##n = {.bus = I2C_DT_SPEC_INST_GET(n)};    \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, hs300x_init, NULL, &hs300x_data_##n, &hs300x_config_##n,   \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     &hs300x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_HS300X)

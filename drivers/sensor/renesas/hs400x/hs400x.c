/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_hs400x

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#define CRC_POLYNOMIAL 0x1D
#define CRC_INITIAL 0xFF

LOG_MODULE_REGISTER(HS400X, CONFIG_SENSOR_LOG_LEVEL);

struct hs400x_config {
	struct i2c_dt_spec bus;
};

struct hs400x_data {
	int16_t t_sample;
	uint16_t rh_sample;
};

static int hs400x_read_sample(const struct device *dev, uint16_t *t_sample, uint16_t *rh_sample)
{
	const struct hs400x_config *cfg = dev->config;
	uint8_t rx_buf[5];
	int rc;

	rc = i2c_read_dt(&cfg->bus, rx_buf, sizeof(rx_buf));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	*rh_sample = sys_get_be16(rx_buf);
	*t_sample = sys_get_be16(&rx_buf[2]);

	/*
	 * The sensor sends a checkum after each measurement. See datasheet "CRC Checksum
	 * Calculation" section for more details on checking the checksum.
	 */
#if CONFIG_HS400X_CRC
	uint8_t crc = crc8(rx_buf, 4, CRC_POLYNOMIAL, CRC_INITIAL, 0);

	if (crc != rx_buf[4]) {
		LOG_ERR("CRC check failed: computed=%u,expected=%u", crc, rx_buf[4]);
		return -EIO;
	}
#endif

	return 0;
}

static int hs400x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct hs400x_data *data = dev->data;
	const struct hs400x_config *cfg = dev->config;
	int rc;
	uint8_t no_hold_measurement = 0xF5;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	rc = i2c_write_dt(&cfg->bus, (const uint8_t *)&no_hold_measurement, 1);
	if (rc < 0) {
		LOG_ERR("Failed to send measurement.");
		return rc;
	}

	/*
	 * According to datasheet maximum time to make temperature and humidity
	 * measurements is 1.7 ms, add a little safety margin...
	 */
	k_msleep(3);

	rc = hs400x_read_sample(dev, &data->t_sample, &data->rh_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	return 0;
}

static void hs400x_temp_convert(struct sensor_value *val, int16_t raw)
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

static void hs400x_rh_convert(struct sensor_value *val, uint16_t raw)
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

static int hs400x_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct hs400x_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		hs400x_temp_convert(val, data->t_sample);
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		hs400x_rh_convert(val, data->rh_sample);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void hs400x_all_measurements_stop(const struct device *dev)
{
	const struct hs400x_config *cfg = dev->config;
	const uint8_t periodic_measurement_stop = 0x30;
	uint8_t dummy[2];

	/*
	 * Stop previous periodic measurement.
	 * If a periodic measurement is not running, HS400x device replies with NACK.
	 */
	i2c_write_dt(&cfg->bus, &periodic_measurement_stop, 1);

	/*
	 * Clear previous no-hold measurement.
	 * If a measurement is not complete, HS400x device replies with NACK.
	 */
	i2c_read_dt(&cfg->bus, dummy, 2);
}

static int hs400x_init(const struct device *dev)
{
	const struct hs400x_config *cfg = dev->config;
	const uint8_t reset_command = 0xFE;
	int rc;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	hs400x_all_measurements_stop(dev);

	rc = i2c_write_dt(&cfg->bus, &reset_command, 1);
	if (rc < 0) {
		LOG_ERR("Failed to send reset command.");
		return rc;
	}
	k_msleep(500);

	return 0;
}

static DEVICE_API(sensor, hs400x_driver_api) = {
	.sample_fetch = hs400x_sample_fetch,
	.channel_get = hs400x_channel_get,
};

#define DEFINE_HS400X(n)                                                                           \
	static struct hs400x_data hs400x_data_##n;                                                 \
                                                                                                   \
	static const struct hs400x_config hs400x_config_##n = {.bus = I2C_DT_SPEC_INST_GET(n)};    \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, hs400x_init, NULL, &hs400x_data_##n, &hs400x_config_##n,   \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     &hs400x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_HS400X)

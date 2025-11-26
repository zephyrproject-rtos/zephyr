/*
 * Copyright (c) 2024 LCKFB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gxcas_gxhtc3

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "gxhtc3.h"

LOG_MODULE_REGISTER(GXHTC3, CONFIG_SENSOR_LOG_LEVEL);

/* Calculate CRC-8 checksum */
static uint8_t gxhtc3_calc_crc(uint8_t *data, uint8_t len)
{
	uint8_t crc = 0xFF;
	uint8_t i, j;

	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (j = 8; j > 0; --j) {
			if (crc & 0x80) {
				crc = (crc << 1) ^ GXHTC3_CRC_POLYNOMIAL;
			} else {
				crc = (crc << 1);
			}
		}
	}
	return crc;
}

/* Read chip ID */
static int gxhtc3_read_id(const struct device *dev)
{
	const struct gxhtc3_config *config = dev->config;
	uint8_t cmd[2] = {GXHTC3_CMD_READ_ID_MSB, GXHTC3_CMD_READ_ID_LSB};
	uint8_t data[3];
	int ret;

	/* Write read ID command */
	ret = i2c_write_dt(&config->i2c, cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Failed to write read ID command: %d", ret);
		return ret;
	}

	/* Small delay to ensure command is processed */
	k_sleep(K_MSEC(10));

	/* Read ID data */
	ret = i2c_read_dt(&config->i2c, data, sizeof(data));
	if (ret < 0) {
		LOG_ERR("Failed to read ID: %d", ret);
		return ret;
	}

	/* Verify CRC */
	if (data[2] != gxhtc3_calc_crc(data, 2)) {
		LOG_ERR("ID CRC check failed: got 0x%02x, expected 0x%02x",
			data[2], gxhtc3_calc_crc(data, 2));
		return -EIO;
	}

	LOG_DBG("GXHTC3 ID read successfully");
	return 0;
}

/* Wake up sensor */
static int gxhtc3_wake_up(const struct device *dev)
{
	const struct gxhtc3_config *config = dev->config;
	uint8_t cmd[2] = {GXHTC3_CMD_WAKE_UP_MSB, GXHTC3_CMD_WAKE_UP_LSB};
	int ret;

	ret = i2c_write_dt(&config->i2c, cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Failed to wake up sensor");
		return ret;
	}

	return 0;
}

/* Start measurement */
static int gxhtc3_measure(const struct device *dev)
{
	const struct gxhtc3_config *config = dev->config;
	uint8_t cmd[2] = {GXHTC3_CMD_MEASURE_MSB, GXHTC3_CMD_MEASURE_LSB};
	int ret;

	ret = i2c_write_dt(&config->i2c, cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Failed to start measurement");
		return ret;
	}

	return 0;
}

/* Read temperature and humidity data */
static int gxhtc3_read_tah(const struct device *dev, uint8_t *data)
{
	const struct gxhtc3_config *config = dev->config;
	int ret;

	ret = i2c_read_dt(&config->i2c, data, 6);
	if (ret < 0) {
		LOG_ERR("Failed to read TAH data");
		return ret;
	}

	return 0;
}

/* Put sensor to sleep */
static int gxhtc3_sleep(const struct device *dev)
{
	const struct gxhtc3_config *config = dev->config;
	uint8_t cmd[2] = {GXHTC3_CMD_SLEEP_MSB, GXHTC3_CMD_SLEEP_LSB};
	int ret;

	ret = i2c_write_dt(&config->i2c, cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Failed to put sensor to sleep");
		return ret;
	}

	return 0;
}

/* Sample fetch implementation */
static int gxhtc3_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct gxhtc3_data *drv_data = dev->data;
	uint8_t tah_data[6];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Wake up sensor */
	ret = gxhtc3_wake_up(dev);
	if (ret < 0) {
		return ret;
	}

	/* Start measurement */
	ret = gxhtc3_measure(dev);
	if (ret < 0) {
		gxhtc3_sleep(dev);
		return ret;
	}

	/* Wait for measurement to complete */
	k_sleep(K_MSEC(GXHTC3_MEASURE_DELAY_MS));

	/* Read data */
	ret = gxhtc3_read_tah(dev, tah_data);
	if (ret < 0) {
		gxhtc3_sleep(dev);
		return ret;
	}

	/* Put sensor to sleep */
	ret = gxhtc3_sleep(dev);
	if (ret < 0) {
		LOG_WRN("Failed to put sensor to sleep");
	}

	/* Verify CRC */
	if (tah_data[2] != gxhtc3_calc_crc(tah_data, 2) ||
	    tah_data[5] != gxhtc3_calc_crc(&tah_data[3], 2)) {
		LOG_ERR("TAH data CRC check failed");
		drv_data->raw_temp = 0;
		drv_data->raw_humi = 0;
		drv_data->temperature = 0.0f;
		drv_data->humidity = 0.0f;
		return -EIO;
	}

	/* Extract raw values */
	drv_data->raw_temp = (tah_data[0] << 8) | tah_data[1];
	drv_data->raw_humi = (tah_data[3] << 8) | tah_data[4];

	/* Convert to physical values */
	drv_data->temperature = (175.0f * (float)drv_data->raw_temp) / 65535.0f - 45.0f;
	drv_data->humidity = (100.0f * (float)drv_data->raw_humi) / 65535.0f;

	return 0;
}

/* Channel get implementation */
static int gxhtc3_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct gxhtc3_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = (int32_t)drv_data->temperature;
		val->val2 = (int32_t)((drv_data->temperature - (float)val->val1) * 1000000.0f);
		break;

	case SENSOR_CHAN_HUMIDITY:
		val->val1 = (int32_t)drv_data->humidity;
		val->val2 = (int32_t)((drv_data->humidity - (float)val->val1) * 1000000.0f);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api gxhtc3_driver_api = {
	.sample_fetch = gxhtc3_sample_fetch,
	.channel_get = gxhtc3_channel_get,
};

/* Initialize sensor */
static int gxhtc3_init(const struct device *dev)
{
	const struct gxhtc3_config *config = dev->config;
	int ret;
	int retry_count = 5;

	LOG_DBG("Initializing GXHTC3 on I2C bus %s, address 0x%02x",
		config->i2c.bus->name, config->i2c.addr);

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready: %s", config->i2c.bus->name);
		return -ENODEV;
	}

	LOG_DBG("I2C bus is ready");

	/* Small delay to ensure I2C bus is stable */
	k_sleep(K_MSEC(100));

	/* Read chip ID to verify communication with retry */
	for (int i = 0; i < retry_count; i++) {
		ret = gxhtc3_read_id(dev);
		if (ret == 0) {
			LOG_INF("GXHTC3 initialized successfully");
			return 0;
		}
		LOG_WRN("Failed to read chip ID (attempt %d/%d), retrying...",
			i + 1, retry_count);
		k_sleep(K_MSEC(100));
	}

	LOG_ERR("Failed to read chip ID after %d attempts", retry_count);
	return ret;
}

#define GXHTC3_DEFINE(inst)							\
	static struct gxhtc3_data gxhtc3_data_##inst;				\
										\
	static const struct gxhtc3_config gxhtc3_config_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, gxhtc3_init, NULL,			\
				      &gxhtc3_data_##inst,			\
				      &gxhtc3_config_##inst, POST_KERNEL,	\
				      CONFIG_SENSOR_INIT_PRIORITY,		\
				      &gxhtc3_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(GXHTC3_DEFINE)


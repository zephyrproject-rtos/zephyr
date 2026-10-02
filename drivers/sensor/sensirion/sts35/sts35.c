/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Milan Pipaliya <milan.pipaliya@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for the Sensirion STS35-DIS high-accuracy digital
 *        temperature sensor.
 *
 * The STS35-DIS communicates over I2C using 16-bit commands and returns
 * a 16-bit temperature word followed by an 8-bit CRC checksum. This
 * driver implements the sensor subsystem's standard single-shot
 * fetch/get model: sts35_sample_fetch() triggers a single-shot
 * measurement without clock stretching and blocks for the worst-case
 * conversion time, sts35_channel_get() then converts the last raw
 * sample into a struct sensor_value in degrees Celsius.
 */

#define DT_DRV_COMPAT sensirion_sts35

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "sts35.h"

LOG_MODULE_REGISTER(STS35, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Single-shot measurement commands indexed by repeatability.
 */
static const uint16_t sts35_meas_cmd[] = {
	[STS35_REPEATABILITY_LOW] = STS35_CMD_MEAS_LOW,
	[STS35_REPEATABILITY_MEDIUM] = STS35_CMD_MEAS_MEDIUM,
	[STS35_REPEATABILITY_HIGH] = STS35_CMD_MEAS_HIGH,
};

/**
 * @brief Worst case measurement duration (ms) indexed by repeatability.
 */
static const uint16_t sts35_meas_duration_ms[] = {
	[STS35_REPEATABILITY_LOW] = 5U,
	[STS35_REPEATABILITY_MEDIUM] = 7U,
	[STS35_REPEATABILITY_HIGH] = 16U,
};

/**
 * @brief Compute the Sensirion CRC-8 checksum over a data word.
 *
 * @param data Pointer to the data bytes to checksum.
 * @param len Number of bytes pointed to by @p data.
 *
 * @return The computed 8-bit CRC value.
 */
static uint8_t sts35_crc8(const uint8_t *data, size_t len)
{
	uint8_t crc = STS35_CRC8_INIT;

	for (size_t i = 0U; i < len; i++) {
		crc ^= data[i];

		for (int bit = 0; bit < 8; bit++) {
			if (crc & 0x80U) {
				crc = (uint8_t)((crc << 1) ^ STS35_CRC8_POLY);
			} else {
				crc = (uint8_t)(crc << 1);
			}
		}
	}

	return crc;
}

static int sts35_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct sts35_config *cfg = dev->config;
	struct sts35_data *data = dev->data;
	uint8_t cmd[2];
	uint8_t rx[STS35_WORD_CRC_LEN];
	int ret;

	if ((chan != SENSOR_CHAN_AMBIENT_TEMP) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	sys_put_be16(sts35_meas_cmd[cfg->repeatability], cmd);

	ret = i2c_write_dt(&cfg->bus, cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Failed to send measurement command: %d", ret);
		return ret;
	}

	k_sleep(K_MSEC(sts35_meas_duration_ms[cfg->repeatability]));

	ret = i2c_read_dt(&cfg->bus, rx, sizeof(rx));
	if (ret < 0) {
		LOG_ERR("Failed to read measurement data: %d", ret);
		return ret;
	}

	if (sts35_crc8(rx, 2U) != rx[2]) {
		LOG_ERR("CRC check failed for temperature data");
		return -EIO;
	}

	data->sample = sys_get_be16(rx);

	return 0;
}

static int sts35_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct sts35_data *data = dev->data;
	int64_t temp_micro_c;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	temp_micro_c = -45000000LL + ((175000000LL * data->sample) / 65535LL);

	return sensor_value_from_micro(val, temp_micro_c);
}

static int sts35_init(const struct device *dev)
{
	const struct sts35_config *cfg = dev->config;
	uint8_t cmd[2];
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}

	sys_put_be16(STS35_CMD_SOFT_RESET, cmd);

	ret = i2c_write_dt(&cfg->bus, cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Failed to soft reset device: %d", ret);
		return ret;
	}

	k_sleep(K_MSEC(STS35_SOFT_RESET_TIME_MS));

	return 0;
}

static DEVICE_API(sensor, sts35_driver_api) = {
	.sample_fetch = sts35_sample_fetch,
	.channel_get = sts35_channel_get,
};

#define STS35_INIT(inst)                                                                           \
	static struct sts35_data sts35_data_##inst;                                                \
	static const struct sts35_config sts35_config_##inst = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.repeatability = DT_INST_PROP(inst, repeatability),                                \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sts35_init, NULL, &sts35_data_##inst,                   \
				     &sts35_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &sts35_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STS35_INIT)

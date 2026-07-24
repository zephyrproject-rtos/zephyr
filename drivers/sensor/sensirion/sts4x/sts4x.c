/*
 * Copyright (c) 2026 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_sts4x

#include "sts4x.h"
#include "../common/conversions.h"
#include "../common/i2c_packet.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/sensor/sts4x.h>

LOG_MODULE_REGISTER(STS4x, CONFIG_SENSOR_LOG_LEVEL);

static const struct i2c_dt_spec *i2c_spec;

static uint8_t communication_buffer[6] = {0};

static i2c_packet_t packet = {
	.crc8_poly = 0x31, .crc8_init = 0xff, .max_length = 6, .data = communication_buffer};

static uint8_t _repeatability;

int sts4x_measure_temperature_high_precision(uint16_t *temperature_ticks)
{
	int ret = NO_ERROR;
	uint16_t local_offset = 0U;

	local_offset = sensirion_i2c_packet_add_command8(
		&packet, local_offset, STS4X_MEASURE_TEMPERATURE_HIGH_PRECISION_CMD_ID);
	ret = sensirion_i2c_packet_write(&packet, local_offset, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	k_msleep(9U);
	ret = sensirion_i2c_packet_read(&packet, 2, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	*temperature_ticks = sensirion_conversions_bytes_to_uint16_t(&packet.data[0]);
	return ret;
}

int sts4x_measure_temperature_medium_precision(uint16_t *temperature_ticks)
{
	int ret = NO_ERROR;
	uint16_t local_offset = 0U;

	local_offset = sensirion_i2c_packet_add_command8(
		&packet, local_offset, STS4X_MEASURE_TEMPERATURE_MEDIUM_PRECISION_CMD_ID);
	ret = sensirion_i2c_packet_write(&packet, local_offset, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	k_msleep(5U);
	ret = sensirion_i2c_packet_read(&packet, 2, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	*temperature_ticks = sensirion_conversions_bytes_to_uint16_t(&packet.data[0]);
	return ret;
}

int sts4x_measure_temperature_low_precision(uint16_t *temperature_ticks)
{
	int ret = NO_ERROR;
	uint16_t local_offset = 0U;

	local_offset = sensirion_i2c_packet_add_command8(
		&packet, local_offset, STS4X_MEASURE_TEMPERATURE_LOW_PRECISION_CMD_ID);
	ret = sensirion_i2c_packet_write(&packet, local_offset, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	k_msleep(2U);
	ret = sensirion_i2c_packet_read(&packet, 2, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	*temperature_ticks = sensirion_conversions_bytes_to_uint16_t(&packet.data[0]);
	return ret;
}

int sts4x_read_serial_number(uint32_t *serial_number)
{
	int ret = NO_ERROR;
	uint16_t local_offset = 0U;

	local_offset = sensirion_i2c_packet_add_command8(&packet, local_offset,
							 STS4X_READ_SERIAL_NUMBER_CMD_ID);
	ret = sensirion_i2c_packet_write(&packet, local_offset, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	k_msleep(1U);
	ret = sensirion_i2c_packet_read(&packet, 4, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	*serial_number = sensirion_conversions_bytes_to_uint32_t(&packet.data[0]);
	return ret;
}

int sts4x_soft_reset(void)
{
	int ret = NO_ERROR;
	uint16_t local_offset = 0U;

	local_offset =
		sensirion_i2c_packet_add_command8(&packet, local_offset, STS4X_SOFT_RESET_CMD_ID);
	ret = sensirion_i2c_packet_write(&packet, local_offset, i2c_spec);
	if (ret != NO_ERROR) {
		return ret;
	}
	k_msleep(1U);
	return ret;
}

float sts4x_signal_ticks_to_celsius(uint16_t temperature_ticks)
{
	float ticks = 0.0;
	float ticks_to_celsius = 0.0;

	ticks = (float)(temperature_ticks);
	ticks_to_celsius = -45.0f + ((175.0f * ticks) / 65535.0f);
	return ticks_to_celsius;
}

float sts4x_signal_ticks_to_fahrenheit(uint16_t temperature_ticks)
{
	float ticks = 0.0;
	float ticks_to_fahrenheit = 0.0;

	ticks = (float)(temperature_ticks);
	ticks_to_fahrenheit = -49.0f + ((315.0f * ticks) / 65535.0f);
	return ticks_to_fahrenheit;
}

void sts4x_set_repeatability(uint8_t rep)
{
	if (rep > 2) {
		return;
	}
	_repeatability = rep;
}

int sts4x_read_measurement_raw(uint16_t *temperature_ticks)
{
	int ret = 0;

	if (_repeatability == 0) {
		ret = sts4x_measure_temperature_low_precision(temperature_ticks);
		return ret;
	} else if (_repeatability == 1) {
		ret = sts4x_measure_temperature_medium_precision(temperature_ticks);
		return ret;
	} else if (_repeatability == 2) {
		ret = sts4x_measure_temperature_high_precision(temperature_ticks);
		return ret;
	}
	return ret;
}

static int sts4x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct sts4x_data *data = dev->data;
	int ret = 0;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP || chan == SENSOR_CHAN_ALL) {
		ret = sts4x_read_measurement_raw(&data->temperature_ticks);
	} else {
		LOG_ERR("Channel not supported.");
		return -ENOTSUP;
	}

	if (ret != NO_ERROR) {
		LOG_ERR("Failed to sample fetch.");
		return ret;
	}
	return 0;
}

static int sts4x_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct sts4x_data *data = dev->data;
	int ret = 0;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		float local_var = sts4x_signal_ticks_to_celsius(data->temperature_ticks);

		ret = sensor_value_from_float(val, local_var);
	} else {
		LOG_ERR("Channel not supported.");
		return -ENOTSUP;
	}

	if (ret != NO_ERROR) {
		LOG_ERR("Failed to convert value.");
		return ret;
	}

	return 0;
}

int sts4x_init(const struct device *dev)
{
	int ret = 0;

	const struct sts4x_config *cfg = dev->config;

	i2c_spec = &cfg->bus;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	sts4x_set_repeatability(cfg->repeatability);
	ret = sts4x_soft_reset();
	if (ret != NO_ERROR) {
		LOG_ERR("error executing soft_reset(): %i\n", ret);
		return ret;
	}
	return 0;
}

static DEVICE_API(sensor, sts4x_api) = {
	.sample_fetch = sts4x_sample_fetch,
	.channel_get = sts4x_channel_get,
};

#define STS4x_INIT(inst)                                                                           \
	static struct sts4x_data sts4x_data_##inst;                                                \
                                                                                                   \
	static const struct sts4x_config sts4x_config_##inst = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.repeatability = DT_INST_PROP(inst, repeatability),                                \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sts4x_init, NULL, &sts4x_data_##inst,                   \
				     &sts4x_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &sts4x_api);

DT_INST_FOREACH_STATUS_OKAY(STS4x_INIT)

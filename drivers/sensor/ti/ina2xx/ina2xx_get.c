/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina2xx_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

static int ina2xx_get_bus_voltage(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->bus_voltage;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	ina2xx_sensor_value_from_channel_s32(val, ch, data->bus_voltage);
	return 0;
}

static int ina2xx_get_shunt_voltage(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->shunt_voltage;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	ina2xx_sensor_value_from_channel_s32(val, ch, data->shunt_voltage);
	return 0;
}

static int ina2xx_get_current(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->current;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	const int32_t raw = data->current * config->current_lsb;

	ina2xx_sensor_value_from_channel_s32(val, ch, raw);
	return 0;
}

static int ina2xx_get_power(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->power;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	const uint32_t raw = data->power * config->current_lsb;

	ina2xx_sensor_value_from_channel_u32(val, ch, raw);
	return 0;
}

static int ina2xx_get_die_temp(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->die_temp;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	ina2xx_sensor_value_from_channel_s32(val, ch, data->die_temp);
	return 0;
}

static int ina2xx_get_energy(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->energy;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	const uint64_t raw = data->energy * config->current_lsb;

	ina2xx_sensor_value_from_channel_u64(val, ch, raw);
	return 0;
}

static int ina2xx_get_charge(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->charge;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	const uint64_t raw = data->charge * config->current_lsb;

	ina2xx_sensor_value_from_channel_u64(val, ch, raw);
	return 0;
}

int ina2xx_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	if (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY) {
		return ina2xx_get_energy(dev, val);
	}

	if (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE) {
		return ina2xx_get_charge(dev, val);
	}

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		return ina2xx_get_bus_voltage(dev, val);
	case SENSOR_CHAN_VSHUNT:
		return ina2xx_get_shunt_voltage(dev, val);
	case SENSOR_CHAN_CURRENT:
		return ina2xx_get_current(dev, val);
	case SENSOR_CHAN_POWER:
		return ina2xx_get_power(dev, val);
	case SENSOR_CHAN_DIE_TEMP:
		return ina2xx_get_die_temp(dev, val);
	default:
		return -ENOTSUP;
	}
}

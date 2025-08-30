/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina2xx_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

static int ina2xx_read_bus_voltage(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->bus_voltage;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_read(&config->bus, ch->reg, &data->bus_voltage);
}

static int ina2xx_read_shunt_voltage(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->shunt_voltage;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_read(&config->bus, ch->reg, &data->shunt_voltage);
}

static int ina2xx_read_current(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->current;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_read(&config->bus, ch->reg, &data->current);
}

static int ina2xx_read_power(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->power;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_read(&config->bus, ch->reg, &data->power);
}

static int ina2xx_read_die_temp(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->die_temp;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_read(&config->bus, ch->reg, &data->die_temp);
}

static int ina2xx_read_energy(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->energy;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_read(&config->bus, ch->reg, &data->energy);
}

static int ina2xx_read_charge(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->charge;
	struct ina2xx_data *data = dev->data;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_read(&config->bus, ch->reg, &data->charge);
}

int ina2xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = -ENOTSUP;

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VSHUNT)) {
		ret = ina2xx_read_shunt_voltage(dev);
		if (ret < 0 && chan != SENSOR_CHAN_ALL) {
			LOG_ERR("failed to read shunt voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_VOLTAGE)) {
		ret = ina2xx_read_bus_voltage(dev);
		if (ret < 0 && chan != SENSOR_CHAN_ALL) {
			LOG_ERR("failed to read bus voltage");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_CURRENT)) {
		ret = ina2xx_read_current(dev);
		if (ret < 0 && chan != SENSOR_CHAN_ALL) {
			LOG_ERR("failed to read current");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_POWER)) {
		ret = ina2xx_read_power(dev);
		if (ret < 0 && chan != SENSOR_CHAN_ALL) {
			LOG_ERR("failed to read power");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP)) {
		ret = ina2xx_read_die_temp(dev);
		if (ret < 0 && chan != SENSOR_CHAN_ALL) {
			LOG_ERR("failed to read temperature");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY)) {
		ret = ina2xx_read_energy(dev);
		if (ret < 0 && chan != SENSOR_CHAN_ALL) {
			LOG_ERR("failed to read energy accumulator");
			return ret;
		}
	}

	if ((chan == SENSOR_CHAN_ALL) || (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE)) {
		ret = ina2xx_read_charge(dev);
		if (ret < 0 && chan != SENSOR_CHAN_ALL) {
			LOG_ERR("failed to read charge accumulator");
			return ret;
		}
	}

	return (chan == SENSOR_CHAN_ALL) ? 0 : ret;
}

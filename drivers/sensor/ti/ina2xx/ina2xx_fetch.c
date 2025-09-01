/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina2xx_common.h"

static int ina2xx_fetch_bus_voltage(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_BUS_VOLTAGE)) {
		const struct ina2xx_config *config = dev->config;
		const struct ina2xx_channel *ch = config->channels->voltage;
		struct ina2xx_data *data = dev->data;

		if (ch == NULL) {
			return -ENOTSUP;
		}

		return ina2xx_reg_read(&config->bus, ch->reg, data->voltage, sizeof(data->voltage));
	}

	return -ENOTSUP;
}

static int ina2xx_fetch_shunt_voltage(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_SHUNT_VOLTAGE)) {
		const struct ina2xx_config *config = dev->config;
		const struct ina2xx_channel *ch = config->channels->vshunt;
		struct ina2xx_data *data = dev->data;

		if (ch == NULL) {
			return -ENOTSUP;
		}

		return ina2xx_reg_read(&config->bus, ch->reg, data->vshunt, sizeof(data->vshunt));
	}

	return -ENOTSUP;
}

static int ina2xx_fetch_current(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_CURRENT)) {
		const struct ina2xx_config *config = dev->config;
		const struct ina2xx_channel *ch = config->channels->current;
		struct ina2xx_data *data = dev->data;

		if (ch == NULL) {
			return -ENOTSUP;
		}

		return ina2xx_reg_read(&config->bus, ch->reg, data->current, sizeof(data->current));
	}

	return -ENOTSUP;
}

static int ina2xx_fetch_power(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_POWER)) {
		const struct ina2xx_config *config = dev->config;
		const struct ina2xx_channel *ch = config->channels->power;
		struct ina2xx_data *data = dev->data;

		if (ch == NULL) {
			return -ENOTSUP;
		}

		return ina2xx_reg_read(&config->bus, ch->reg, data->power, sizeof(data->power));
	}

	return -ENOTSUP;
}

static int ina2xx_fetch_die_temp(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_DIE_TEMP)) {
		const struct ina2xx_config *config = dev->config;
		const struct ina2xx_channel *ch = config->channels->die_temp;
		struct ina2xx_data *data = dev->data;

		if (ch == NULL) {
			return -ENOTSUP;
		}

		return ina2xx_reg_read(&config->bus, ch->reg, data->die_temp,
			sizeof(data->die_temp));
	}

	return -ENOTSUP;
}

static int ina2xx_fetch_energy(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_ENERGY)) {
		const struct ina2xx_config *config = dev->config;
		const struct ina2xx_channel *ch = config->channels->energy;
		struct ina2xx_data *data = dev->data;

		if (ch == NULL) {
			return -ENOTSUP;
		}

		return ina2xx_reg_read(&config->bus, ch->reg, data->energy, sizeof(data->energy));
	}

	return -ENOTSUP;
}

static int ina2xx_fetch_charge(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INA2XX_HAS_CHANNEL_CHARGE)) {
		const struct ina2xx_config *config = dev->config;
		const struct ina2xx_channel *ch = config->channels->charge;
		struct ina2xx_data *data = dev->data;

		if (ch == NULL) {
			return -ENOTSUP;
		}

		return ina2xx_reg_read(&config->bus, ch->reg, data->charge, sizeof(data->charge));
	}

	return -ENOTSUP;
}

static int ina2xx_fetch_all(const struct device *dev)
{
	int ret;

	ret = ina2xx_fetch_bus_voltage(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		return ret;
	}

	ret = ina2xx_fetch_shunt_voltage(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		return ret;
	}

	ret = ina2xx_fetch_current(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		return ret;
	}

	ret = ina2xx_fetch_power(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		return ret;
	}

	ret = ina2xx_fetch_die_temp(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		return ret;
	}

	ret = ina2xx_fetch_energy(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		return ret;
	}

	ret = ina2xx_fetch_charge(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		return ret;
	}

	return 0;
}

int ina2xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	/* Extended channels */
	if (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY) {
		return ina2xx_fetch_energy(dev);
	} else if (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE) {
		return ina2xx_fetch_charge(dev);
	}

	/* Standard channels */
	switch (chan) {
	case SENSOR_CHAN_ALL:
		return ina2xx_fetch_all(dev);
	case SENSOR_CHAN_VOLTAGE:
		return ina2xx_fetch_bus_voltage(dev);
	case SENSOR_CHAN_VSHUNT:
		return ina2xx_fetch_shunt_voltage(dev);
	case SENSOR_CHAN_CURRENT:
		return ina2xx_fetch_current(dev);
	case SENSOR_CHAN_POWER:
		return ina2xx_fetch_power(dev);
	case SENSOR_CHAN_DIE_TEMP:
		return ina2xx_fetch_die_temp(dev);
	default:
		return -ENOTSUP;
	}
}

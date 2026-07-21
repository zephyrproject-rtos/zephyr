/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina2xx_common.h"

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

static int ina2xx_get_bus_voltage(const struct device *dev, struct sensor_value *val)
{
#ifdef CONFIG_INA2XX_HAS_CHANNEL_BUS_VOLTAGE
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->voltage;
	struct ina2xx_data *data = dev->data;
	size_t bytes;
	union {
		uint32_t u32;
		int32_t s32;
	} value;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	bytes = (ch->reg->size + 7) / 8;

	/* 16 or 20 bit, two's complement */
	if (bytes == 2) {
		value.u32 = sys_get_be16(data->voltage) >> ch->shift;
		value.s32 = sign_extend(value.u32, (15 - ch->shift));
	} else if (bytes == 3) {
		value.u32 = sys_get_be24(data->voltage) >> ch->shift;
		value.s32 = sign_extend(value.u32, (23 - ch->shift));
	} else {
		return -ENOTSUP;
	}

	value.s32 = (ch->mult * value.s32) / ch->div;

	return sensor_value_from_micro(val, value.s32);
#else
	return -ENOTSUP;
#endif /* CONFIG_INA2XX_HAS_CHANNEL_BUS_VOLTAGE */
}

static int ina2xx_get_shunt_voltage(const struct device *dev, struct sensor_value *val)
{
#ifdef CONFIG_INA2XX_HAS_CHANNEL_SHUNT_VOLTAGE
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->vshunt;
	struct ina2xx_data *data = dev->data;
	size_t bytes;
	union {
		uint32_t u32;
		int32_t s32;
	} value;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	bytes = (ch->reg->size + 7) / 8;

	/* 16 or 20 bit, two's complement */
	if (bytes == 2) {
		value.u32 = sys_get_be16(data->vshunt) >> ch->shift;
		value.s32 = sign_extend(value.u32, (15 - ch->shift));
	} else if (bytes == 3) {
		value.u32 = sys_get_be24(data->vshunt) >> ch->shift;
		value.s32 = sign_extend(value.u32, (23 - ch->shift));
	} else {
		return -ENOTSUP;
	}

	value.s32 = (ch->mult * value.s32) / ch->div;

	return sensor_value_from_micro(val, value.s32);
#else
	return -ENOTSUP;
#endif /* CONFIG_INA2XX_HAS_CHANNEL_SHUNT_VOLTAGE */
}

static int ina2xx_get_current(const struct device *dev, struct sensor_value *val)
{
#ifdef CONFIG_INA2XX_HAS_CHANNEL_CURRENT
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->current;
	struct ina2xx_data *data = dev->data;
	size_t bytes;
	union {
		uint32_t u32;
		int32_t s32;
	} value;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	bytes = (ch->reg->size + 7) / 8;

	/* 16 or 20 bit, two's complement. Multiplied by current lsb */
	if (bytes == 2) {
		value.u32 = sys_get_be16(data->current) >> ch->shift;
		value.s32 = sign_extend(value.u32, (15 - ch->shift));
	} else if (bytes == 3) {
		value.u32 = sys_get_be24(data->current) >> ch->shift;
		value.s32 = sign_extend(value.u32, (23 - ch->shift));
	} else {
		return -ENOTSUP;
	}

	value.s32 = ((config->current_lsb * value.s32) / ch->div) * ch->mult;

	return sensor_value_from_micro(val, value.s32);
#else
	return -ENOTSUP;
#endif /* CONFIG_INA2XX_HAS_CHANNEL_CURRENT */
}

static int ina2xx_get_power(const struct device *dev, struct sensor_value *val)
{
#ifdef CONFIG_INA2XX_HAS_CHANNEL_POWER
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->power;
	struct ina2xx_data *data = dev->data;
	size_t bytes;
	uint64_t value;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	bytes = (ch->reg->size + 7) / 8;

	/* 16 or 24 bit, unsigned. Multiplied by current lsb */
	if (bytes == 2) {
		value = sys_get_be16(data->power) >> ch->shift;
	} else if (bytes == 3) {
		value = sys_get_be24(data->power) >> ch->shift;
	} else {
		return -ENOTSUP;
	}

	value = ((config->current_lsb * value) / ch->div) * ch->mult;

	return sensor_value_from_micro(val, value);
#else
	return -ENOTSUP;
#endif /* CONFIG_INA2XX_HAS_CHANNEL_POWER */
}

static int ina2xx_get_die_temp(const struct device *dev, struct sensor_value *val)
{
#ifdef CONFIG_INA2XX_HAS_CHANNEL_DIE_TEMP
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->die_temp;
	struct ina2xx_data *data = dev->data;
	size_t bytes;
	union {
		uint64_t u64;
		int64_t s64;
	} value;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	bytes = (ch->reg->size + 7) / 8;

	/* 12 or 16 bit, two's complement. */
	if (bytes == 2) {
		value.u64 = sys_get_be16(data->die_temp) >> ch->shift;
		value.s64 = sign_extend_64(value.u64, (15 - ch->shift));
	} else {
		return -ENOTSUP;
	}

	value.s64 = (ch->mult * value.s64) / ch->div;

	return sensor_value_from_micro(val, value.s64);
#else
	return -ENOTSUP;
#endif /* CONFIG_INA2XX_HAS_CHANNEL_DIE_TEMP */
}

static int ina2xx_get_energy(const struct device *dev, struct sensor_value *val)
{
#ifdef CONFIG_INA2XX_HAS_CHANNEL_ENERGY
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->energy;
	struct ina2xx_data *data = dev->data;
	size_t bytes;
	uint64_t value;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	bytes = (ch->reg->size + 7) / 8;

	/* 40 bit, unsigned. Multiplied by current lsb */
	if (bytes == 5) {
		value = sys_get_be40(data->energy) >> ch->shift;
	} else {
		return -ENOTSUP;
	}

	value = ((value * config->current_lsb) / ch->div) * ch->mult;

	return sensor_value_from_micro(val, value);
#else
	return -ENOTSUP;
#endif /* CONFIG_INA2XX_HAS_CHANNEL_ENERGY */
}

static int ina2xx_get_charge(const struct device *dev, struct sensor_value *val)
{
#ifdef CONFIG_INA2XX_HAS_CHANNEL_CHARGE
	const struct ina2xx_config *config = dev->config;
	const struct ina2xx_channel *ch = config->channels->charge;
	struct ina2xx_data *data = dev->data;
	size_t bytes;
	union {
		uint64_t u64;
		int64_t s64;
	} value;

	if (ch == NULL) {
		return -ENOTSUP;
	}

	bytes = (ch->reg->size + 7) / 8;

	/* 40 bit, two's complement. Multiplied by current lsb */
	if (bytes == 5) {
		value.u64 = sys_get_be40(data->charge) >> ch->shift;
		value.s64 = sign_extend_64(value.u64, (39 - ch->shift));
	} else {
		return -ENOTSUP;
	}

	value.s64 = ((config->current_lsb * value.s64) / ch->div) * ch->mult;

	return sensor_value_from_micro(val, value.s64);
#else
	return -ENOTSUP;
#endif /* CONFIG_INA2XX_HAS_CHANNEL_CHARGE */
}

int ina2xx_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	/* Extended channels */
	if (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY) {
		return ina2xx_get_energy(dev, val);
	} else if (chan == (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE) {
		return ina2xx_get_charge(dev, val);
	}

	/* Standard channels */
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

/*
 * Copyright 2021 Grinn
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina2xx_common.h"

#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Value of the manufacturer ID register (ASCII 'TI')
 */
#define INA2XX_MANUFACTURER_ID 0x5449


void ina2xx_sensor_value_from_channel_s32(struct sensor_value *val,
	const struct ina2xx_channel *ch, int32_t raw)
{
	const int32_t cooked = ((int32_t)(raw * ch->mult) / ch->div);

	val->val1 = cooked / 1000000L;
	val->val2 = cooked % 1000000L;
}

void ina2xx_sensor_value_from_channel_u32(struct sensor_value *val,
	const struct ina2xx_channel *ch, uint32_t raw)
{
	const uint32_t cooked = ((uint32_t)(raw * ch->mult) / ch->div);

	val->val1 = cooked / 1000000U;
	val->val2 = cooked % 1000000U;
}

void ina2xx_sensor_value_from_channel_u64(struct sensor_value *val,
	const struct ina2xx_channel *ch, uint64_t raw)
{
	const uint64_t cooked = ((uint64_t)(raw * ch->mult) / ch->div);

	val->val1 = cooked / 1000000U;
	val->val2 = cooked % 1000000U;
}

int ina2xx_reg_read_40(const struct i2c_dt_spec *bus, uint8_t reg, uint64_t *val)
{
	uint8_t data[5];
	int ret;

	ret = i2c_burst_read_dt(bus, reg, data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be40(data);

	return ret;
}

int ina2xx_reg_read_24(const struct i2c_dt_spec *bus, uint8_t reg, uint32_t *val)
{
	uint8_t data[3];
	int ret;

	ret = i2c_burst_read_dt(bus, reg, data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be24(data);

	return ret;
}

int ina2xx_reg_read_16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t *val)
{
	uint8_t data[2];
	int ret;

	ret = i2c_burst_read_dt(bus, reg, data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be16(data);

	return ret;
}

int ina2xx_reg_read(const struct i2c_dt_spec *bus, const struct ina2xx_reg *reg, void *val)
{
	int ret;
	union {
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
	} data;

	switch (reg->size) {
	case 16:
		ret = ina2xx_reg_read_16(bus, reg->addr, &data.u16);
		if (ret == 0) {
			*(uint16_t *)val = data.u16 >> reg->shift;
		}
		break;
	case 24:
		ret = ina2xx_reg_read_24(bus, reg->addr, &data.u32);
		if (ret == 0) {
			*(uint32_t *)val = data.u32 >> reg->shift;
		}
		break;
	case 40:
		ret = ina2xx_reg_read_40(bus, reg->addr, &data.u64);
		if (ret == 0) {
			*(uint64_t *)val = data.u64 >> reg->shift;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

int ina2xx_reg_write(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t val)
{
	uint8_t tx_buf[3];

	tx_buf[0] = reg;
	sys_put_be16(val, &tx_buf[1]);

	return i2c_write_dt(bus, tx_buf, sizeof(tx_buf));
}

int ina2xx_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data = val->val1;

	if (attr == SENSOR_ATTR_CONFIGURATION && config->config_reg->size != 0) {
		return ina2xx_reg_write(&config->bus, config->config_reg->addr, data);
	}

	if (attr == SENSOR_ATTR_CALIBRATION && config->cal_reg->size != 0) {
		return ina2xx_reg_write(&config->bus, config->cal_reg->addr, data);
	}

	return -ENOTSUP;
}

int ina2xx_attr_get(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data;
	int ret;

	if (attr == SENSOR_ATTR_CONFIGURATION && config->config_reg->size != 0) {
		ret = ina2xx_reg_read_16(&config->bus, config->config_reg->addr, &data);
		if (ret < 0) {
			return ret;
		}
	} else {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_CALIBRATION && config->cal_reg->size != 0) {
		ret = ina2xx_reg_read_16(&config->bus, config->cal_reg->addr, &data);
		if (ret < 0) {
			return ret;
		}
	} else {
		return -ENOTSUP;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

int ina2xx_init(const struct device *dev)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t id;
	int ret;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->id_reg->size != 0) {
		ret = ina2xx_reg_read_16(&config->bus, config->id_reg->addr, &id);
		if (ret < 0) {
			LOG_ERR("failed to read manufacturer register");
			return ret;
		}

		if (id != INA2XX_MANUFACTURER_ID) {
			LOG_ERR("manufacturer ID doesn't match");
			return -ENODEV;
		}
	}

	if (config->config_reg->size != 0) {
		ret = ina2xx_reg_write(&config->bus,
			config->config_reg->addr, config->config);

		if (ret < 0) {
			LOG_ERR("failed to write configuration register");
			return ret;
		}
	}

	if (config->adc_config_reg->size != 0) {
		ret = ina2xx_reg_write(&config->bus,
			config->adc_config_reg->addr, config->adc_config);

		if (ret < 0) {
			LOG_ERR("failed to write ADC configuration register");
			return ret;
		}
	}

	if (config->cal_reg->size != 0) {
		ret = ina2xx_reg_write(&config->bus, config->cal_reg->addr, config->cal);
		if (ret < 0) {
			LOG_ERR("failed to write calibration register");
			return ret;
		}
	}

	return 0;
}

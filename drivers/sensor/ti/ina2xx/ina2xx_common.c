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

	if (attr == SENSOR_ATTR_CONFIGURATION && config->config_reg >= 0) {
		return ina2xx_reg_write(&config->bus, config->config_reg, data);
	}

	if (attr == SENSOR_ATTR_CALIBRATION && config->cal_reg >= 0) {
		return ina2xx_reg_write(&config->bus, config->cal_reg, data);
	}

	return -ENOTSUP;
}

int ina2xx_attr_get(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data;
	int ret;

	if (attr == SENSOR_ATTR_CONFIGURATION && config->config_reg >= 0) {
		ret = ina2xx_reg_read_16(&config->bus, config->config_reg, &data);
		if (ret < 0) {
			return ret;
		}
	} else {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_CALIBRATION && config->cal_reg >= 0) {
		ret = ina2xx_reg_read_16(&config->bus, config->cal_reg, &data);
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

	if (config->id_reg >= 0) {
		ret = ina2xx_reg_read_16(&config->bus, config->id_reg, &id);
		if (ret < 0) {
			LOG_ERR("failed to read manufacturer register");
			return ret;
		}

		if (id != INA2XX_MANUFACTURER_ID) {
			LOG_ERR("manufacturer ID doesn't match");
			return -ENODEV;
		}
	}

	if (config->config_reg >= 0) {
		ret = ina2xx_reg_write(&config->bus, config->config_reg, config->config);
		if (ret < 0) {
			LOG_ERR("failed to write configuration register");
			return ret;
		}
	}

	if (config->adc_config_reg >= 0) {
		ret = ina2xx_reg_write(&config->bus, config->adc_config_reg, config->adc_config);
		if (ret < 0) {
			LOG_ERR("failed to write ADC configuration register");
			return ret;
		}
	}

	if (config->cal_reg >= 0) {
		ret = ina2xx_reg_write(&config->bus, config->cal_reg, config->cal);
		if (ret < 0) {
			LOG_ERR("failed to write calibration register");
			return ret;
		}
	}

	return 0;
}

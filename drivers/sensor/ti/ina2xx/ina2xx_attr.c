/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina2xx_common.h"

static int ina2xx_set_adc_config(const struct device *dev, const struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data = val->val1;

	if (config->adc_config_reg == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_write(&config->bus, config->adc_config_reg->addr, data);
}

static int ina2xx_set_config(const struct device *dev, const struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data = val->val1;

	if (config->config_reg == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_write(&config->bus, config->config_reg->addr, data);
}

static int ina2xx_set_calibration(const struct device *dev, const struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data = val->val1;

	if (config->cal_reg == NULL) {
		return -ENOTSUP;
	}

	return ina2xx_reg_write(&config->bus, config->cal_reg->addr, data);
}

int ina2xx_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	if (attr == (enum sensor_attribute)SENSOR_ATTR_ADC_CONFIGURATION) {
		return ina2xx_set_adc_config(dev, val);
	}

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return ina2xx_set_config(dev, val);
	case SENSOR_ATTR_CALIBRATION:
		return ina2xx_set_calibration(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int ina2xx_get_adc_config(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data;
	int ret;

	if (config->adc_config_reg == NULL) {
		return -ENOTSUP;
	}

	ret = ina2xx_reg_read_16(&config->bus, config->adc_config_reg->addr, &data);
	if (ret < 0) {
		return ret;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

static int ina2xx_get_config(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data;
	int ret;

	if (config->config_reg == NULL) {
		return -ENOTSUP;
	}

	ret = ina2xx_reg_read_16(&config->bus, config->config_reg->addr, &data);
	if (ret < 0) {
		return ret;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

static int ina2xx_get_calibration(const struct device *dev, struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	uint16_t data;
	int ret;

	if (config->cal_reg == NULL) {
		return -ENOTSUP;
	}

	ret = ina2xx_reg_read_16(&config->bus, config->cal_reg->addr, &data);
	if (ret < 0) {
		return ret;
	}

	val->val1 = data;
	val->val2 = 0;

	return 0;
}

int ina2xx_attr_get(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val)
{
	if (attr == (enum sensor_attribute)SENSOR_ATTR_ADC_CONFIGURATION) {
		return ina2xx_get_adc_config(dev, val);
	}

	switch (attr) {
	case SENSOR_ATTR_CONFIGURATION:
		return ina2xx_get_config(dev, val);
	case SENSOR_ATTR_CALIBRATION:
		return ina2xx_get_calibration(dev, val);
	default:
		return -ENOTSUP;
	}
}

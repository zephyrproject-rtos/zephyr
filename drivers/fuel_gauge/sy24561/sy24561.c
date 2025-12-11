/*
 * Copyright (c) 2025 Romain Paupe <rpaupe@free.fr>, Franck Duriez <franck.lucien.duriez@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr driver for SY24561 Battery Monitor
 *
 * Datasheet:
 * https://www.silergy.com/download/downloadFile?id=4987&type=product&ftype=note
 */
#include <inttypes.h>
#include <stdint.h>

#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "sy24561.h"

#define DECI_KELVIN_TO_CELSIUS(temp_dk) (((temp_dk) - 2731) / 10)
#define CELSIUS_TO_DECI_KELVIN(temp_c)  ((temp_c) * 10 + 2731)

#define PRIdK(_tp)   PRI##_tp ".%" PRI##_tp "K"
#define dKARGS(_val) (_val / 10), (_val % 10)

#define DT_DRV_COMPAT silergy_sy24561

LOG_MODULE_REGISTER(SY24561, CONFIG_FUEL_GAUGE_LOG_LEVEL);

struct sy24561_config {
	struct i2c_dt_spec i2c;
};

static int sy24561_read_reg(const struct device *dev, uint8_t reg, uint16_t *value)
{
	const struct sy24561_config *config = dev->config;
	uint8_t buffer[2];
	int const ret = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), buffer, sizeof(buffer));

	if (ret != 0) {
		LOG_ERR("i2c_write_read failed (reg 0x%02x): %d", reg, ret);
		return ret;
	}

	*value = sys_get_be16(buffer);
	LOG_DBG("reg[%02x]: %02x %02x => %04x", reg, buffer[0], buffer[1], *value);
	return ret;
}

static int sy24561_write_reg(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct sy24561_config *config = dev->config;
	uint8_t buffer[3] = {reg, 0, 0};

	sys_put_be16(value, buffer + 1);

	int const ret = i2c_write_dt(&config->i2c, buffer, sizeof(buffer));

	if (ret != 0) {
		LOG_ERR("i2c_write_read failed (reg 0x%02x): %d", reg, ret);
		return ret;
	}

	return ret;
}

static int sy24561_get_voltage(const struct device *dev, int *voltage_uV)
{
	uint16_t voltage_reg = 0;
	int const ret = sy24561_read_reg(dev, SY24561_REG_VBAT, &voltage_reg);

	if (ret != 0) {
		return ret;
	}

	/* Scaling formula taken from datasheet at page 5 */
	*voltage_uV = (((voltage_reg * 2500 / 0x1000)) + 2500) * 1000;

	LOG_DBG("voltage: %dmV", *voltage_uV);
	return 0;
}

static int sy24561_get_soc(const struct device *dev, uint8_t *soc_percent)
{
	uint16_t soc_reg = 0;
	int const ret = sy24561_read_reg(dev, SY24561_REG_SOC, &soc_reg);

	if (ret != 0) {
		return ret;
	}

	/* Scaling formula taken from datasheet at page 5 */
	*soc_percent = 100 * soc_reg / 0xffff;

	LOG_DBG("RSOC: %" PRIu8 "%%", *soc_percent);
	return 0;
}

static int sy24561_get_current_direction(const struct device *dev, uint16_t *current_direction)
{
	uint16_t status;
	int const ret = sy24561_read_reg(dev, SY24561_REG_STATUS, &status);

	if (ret != 0) {
		return ret;
	}

	/* This comes from datasheet at page 6 */
	*current_direction = IS_BIT_SET(status, 0);

	return 0;
}

static int sy24561_get_version(const struct device *dev, uint16_t *version)
{
	int const ret = sy24561_read_reg(dev, SY24561_REG_VERSION, version);

	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int sy24561_get_config(const struct device *dev, uint16_t *config)
{
	int const ret = sy24561_read_reg(dev, SY24561_REG_CONFIG, config);

	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int sy24561_get_status(const struct device *dev, uint16_t *fg_status)
{
	uint16_t config;
	int const ret = sy24561_get_config(dev, &config);

	if (ret != 0) {
		LOG_ERR("Failed to read config: %d", ret);
		return ret;
	}

	/* This comes from datasheet at page 6 */
	uint16_t const alarm = IS_BIT_SET(config, 5);

	*fg_status = alarm;

	return ret;
}

/** This function is to reset the alarm bit */
static int sy24561_set_status(const struct device *dev, uint16_t const status)
{
	LOG_DBG("Setting status to %" PRIu16, status);

	if (status != 0) {
		LOG_ERR("Invalid status %" PRIu16 ", it should be 0", status);
		return -EINVAL;
	}

	uint16_t config;
	int const ret = sy24561_get_config(dev, &config);

	if (ret != 0) {
		LOG_ERR("Failed to read config: %d", ret);
		return ret;
	}

	LOG_DBG("config register: 0x%x", config);

	/* This comes from datasheet at page 6 */
	uint16_t const status_mask = 1 << 5;

	config &= ~status_mask;
	LOG_DBG("new config register: 0x%x", config);

	if (sy24561_write_reg(dev, SY24561_REG_CONFIG, config) != 0) {
		LOG_ERR("Impossible to write config register");
		return -ENODEV;
	}

	return 0;
}

static int sy24561_set_alarm_threshold(const struct device *dev, uint16_t percent_threshold)
{
	LOG_DBG("Setting SOC alarm threshold to %" PRIu16, percent_threshold);

	uint16_t config;
	int const ret = sy24561_get_config(dev, &config);

	if (ret != 0) {
		LOG_ERR("Failed to read config: %d", ret);
		return ret;
	}

	LOG_DBG("config register: 0x%x", config);

	/* This comes from datasheet at page 6 */
	uint16_t const percent_threshold_max = 32;
	uint16_t const percent_threshold_min = 1;

	if (percent_threshold > percent_threshold_max) {
		LOG_WRN("SOC alarm threshold %" PRIu16 " should be <= %" PRIu16, percent_threshold,
			percent_threshold_max);
		percent_threshold = percent_threshold_max;
	}

	if (percent_threshold < percent_threshold_min) {
		LOG_WRN("SOC alarm threshold %" PRIu16 " should be >= %" PRIu16, percent_threshold,
			percent_threshold_min);
		percent_threshold = percent_threshold_min;
	}

	/* This comes from datasheet at page 6 */
	uint16_t const alarm_threshold_mask = 0b11111;
	uint16_t const alarm_threshold = 32 - percent_threshold;

	config &= ~alarm_threshold_mask;
	config |= alarm_threshold;
	LOG_DBG("new config register: 0x%x", config);

	if (sy24561_write_reg(dev, SY24561_REG_CONFIG, config) != 0) {
		LOG_ERR("Impossible to write config register");
		return -ENODEV;
	}

	return 0;
}

static int sy24561_set_temperature(const struct device *dev, uint16_t temperature_dK /* 0.1K */)
{
	LOG_DBG("Setting temperature to %" PRIdK(u16), dKARGS(temperature_dK));

	uint16_t config;
	int const ret = sy24561_get_config(dev, &config);

	if (ret != 0) {
		LOG_ERR("Failed to read config: %d", ret);
		return ret;
	}

	/* This comes from datasheet at page 5 */
	uint16_t const temperature_dK_max = CELSIUS_TO_DECI_KELVIN(60);
	uint16_t const temperature_dK_min = CELSIUS_TO_DECI_KELVIN(-20);

	if (temperature_dK > temperature_dK_max) {
		LOG_WRN("Temperature %" PRIdK(u16) " should be <= %" PRIdK(u16),
			dKARGS(temperature_dK), dKARGS(temperature_dK_max));
		temperature_dK = temperature_dK_max;
	}

	if (temperature_dK < temperature_dK_min) {
		LOG_WRN("Temperature %" PRIdK(u16) " should be >= %" PRIdK(u16),
			dKARGS(temperature_dK), dKARGS(temperature_dK_min));
		temperature_dK = temperature_dK_min;
	}

	/* This comes from datasheet at page 5 and 6 */
	uint16_t const temperature_mask = 0xff << 8;
	uint16_t const temperature_reg_value = (40 + DECI_KELVIN_TO_CELSIUS(temperature_dK)) << 8;

	config &= ~temperature_mask;
	config |= temperature_reg_value;
	LOG_DBG("new config register: 0x%x", config);

	if (sy24561_write_reg(dev, SY24561_REG_CONFIG, config) != 0) {
		LOG_ERR("Impossible to write config register");
		return -ENODEV;
	}

	return 0;
}

int sy24561_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
		     union fuel_gauge_prop_val *val)
{
	switch (prop) {
	case FUEL_GAUGE_VOLTAGE:
		return sy24561_get_voltage(dev, &val->voltage);
	case FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE:
		return sy24561_get_soc(dev, &val->relative_state_of_charge);
	case FUEL_GAUGE_STATUS:
		return sy24561_get_status(dev, &val->fg_status);
	case FUEL_GAUGE_CURRENT_DIRECTION:
		return sy24561_get_current_direction(dev, &val->current_direction);
	default:
		LOG_ERR("Property %d not supported", (int)prop);
		return -ENOTSUP;
	}
	return 0;
}

static int sy24561_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
			    union fuel_gauge_prop_val val)
{
	int ret = 0;

	switch (prop) {
	case FUEL_GAUGE_STATE_OF_CHARGE_ALARM:
		return sy24561_set_alarm_threshold(dev, val.state_of_charge_alarm);
	case FUEL_GAUGE_TEMPERATURE:
		return sy24561_set_temperature(dev, val.temperature);
	case FUEL_GAUGE_STATUS:
		return sy24561_set_status(dev, val.fg_status);
	default:
		LOG_ERR("Property %d not supported", (int)prop);
		ret = -ENOTSUP;
	}

	return ret;
}

int sy24561_init(const struct device *dev)
{
	const struct sy24561_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (Z_LOG_CONST_LEVEL_CHECK(LOG_LEVEL_DBG)) {
		uint16_t version;

		sy24561_get_version(dev, &version);
		LOG_DBG("SY24561 version: 0x%x", version);
	}

	return 0;
}

static DEVICE_API(fuel_gauge, sy24561_driver_api) = {
	.set_property = &sy24561_set_prop,
	.get_property = &sy24561_get_prop,
};

#define SY24561_INIT(n)                                                                            \
	static const struct sy24561_config sy24561_config_##n = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &sy24561_init, NULL, NULL, &sy24561_config_##n, POST_KERNEL,      \
			      CONFIG_FUEL_GAUGE_INIT_PRIORITY, &sy24561_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SY24561_INIT)

/*
 * Copyright 2021 Grinn
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA2XX_COMMON_H_
#define ZEPHYR_DRIVERS_SENSOR_INA2XX_COMMON_H_

#include <stdint.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor/ina2xx.h>
#include <zephyr/sys/util_macro.h>

/**
 * @brief Common configuration for INA2xx devices.
 *
 * @param bus I2C bus configuration.
 * @param alert_gpio Alert GPIO pin.
 * @param current_lsb LSB for calculating current in microamps/bit.
 * @param adc_config Initial ADC configuration register value.
 * @param cal Initial calibration register value.
 * @param id_reg Manufacturer ID register address (-1 if not supported).
 * @param adc_config_reg ADC configuration register address (-1 if not supported).
 * @param cal_reg Calibration register address (-1 if not supported).
 */
struct ina2xx_config {
	struct i2c_dt_spec bus;
	uint32_t current_lsb;
	uint16_t config;
	uint16_t adc_config;
	uint16_t cal;
	int id_reg;
	int config_reg;
	int adc_config_reg;
	int cal_reg;
};

int ina2xx_reg_read_40(const struct i2c_dt_spec *bus, uint8_t reg, uint64_t *val);
int ina2xx_reg_read_24(const struct i2c_dt_spec *bus, uint8_t reg, uint32_t *val);
int ina2xx_reg_read_16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t *val);
int ina2xx_reg_write(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t val);

int ina2xx_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val);

int ina2xx_attr_get(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, struct sensor_value *val);

int ina2xx_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_INA2XX_COMMON_H_ */

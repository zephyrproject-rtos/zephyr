/*
 * Original license from ina23x driver:
 *  Copyright 2021 The Chromium OS Authors
 *  Copyright 2021 Grinn
 *
 * Copyright 2024, Remie Lowik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA700_H_
#define ZEPHYR_DRIVERS_SENSOR_INA700_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include "ina700_trigger.h"

#define INA700_REG_CONFIG          0x0
#define INA700_REG_ADC_CONFIG      0x1
#define INA700_REG_BUS_VOLT        0x5
#define INA700_REG_TEMPERATURE     0x6
#define INA700_REG_CURRENT         0x7
#define INA700_REG_POWER           0x8
#define INA700_REG_ENERGY          0x9
#define INA700_REG_CHARGE          0xa
#define INA700_REG_ALERT_DIAG      0xb
#define INA700_REG_COL             0xc
#define INA700_REG_CUL             0xd
#define INA700_REG_BOVL            0xe
#define INA700_REG_BUVL            0xf
#define INA700_REG_TEMP_LIMIT      0x10
#define INA700_REG_PWR_LIMIT       0x11
#define INA700_REG_MANUFACTURER_ID 0x3e

#define INA700_OPER_MODE_SHUTDOWN                                   0x0
#define INA700_OPER_MODE_TRIGGERED_BUS_VOLTAGE                      0x1
#define INA700_OPER_MODE_TRIGGERED_TEMPERATURE                      0x4
#define INA700_OPER_MODE_TRIGGERED_TEMPERATURE_BUS_VOLTAGE          0x5
#define INA700_OPER_MODE_TRIGGERED_TEMPERATURE_CURRENT              0x6
#define INA700_OPER_MODE_TRIGGERED_TEMPERATURE_CURRENT_BUS_VOLTAGE  0x7
#define INA700_OPER_MODE_CONTINUOUS_BUS_VOLTAGE                     0x9
#define INA700_OPER_MODE_CONTINUOUS_TEMPERATURE                     0xc
#define INA700_OPER_MODE_CONTINUOUS_TEMPERATURE_BUS_VOLTAGE         0xd
#define INA700_OPER_MODE_CONTINUOUS_TEMPERATURE_CURRENT             0xe
#define INA700_OPER_MODE_CONTINUOUS_TEMPERATURE_CURRENT_BUS_VOLTAGE 0xf

#define INA700_MANUFACTURER_ID 0x5449

struct ina700_data {
	const struct device *dev;
	int16_t bus_voltage;
	int16_t temperature;
	int16_t current;
	uint32_t power;
	uint64_t energy;
	int64_t charge;
	uint16_t alert_diag;
	enum sensor_channel chan;
	struct ina700_trigger trigger;
};

struct ina700_config {
	struct i2c_dt_spec bus;
	uint16_t config;
	uint16_t adc_config;
	uint16_t alert_diag;
	uint16_t current_over_limit_threshold;
	uint16_t current_under_limit_threshold;
	uint16_t bus_overvoltage_threshold;
	uint16_t bus_undervoltage_threshold;
	uint16_t temperature_over_limit_threshold;
	uint16_t power_over_limit_threshold;
	const struct gpio_dt_spec alert_gpio;
};

int ina700_reg_read_16(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t *val);
int ina700_reg_read_24(const struct i2c_dt_spec *bus, uint8_t reg, uint32_t *val);
int ina700_reg_read_40(const struct i2c_dt_spec *bus, uint8_t reg, uint64_t *val);
int ina700_reg_write(const struct i2c_dt_spec *bus, uint8_t reg, uint16_t val);

#endif /* ZEPHYR_DRIVERS_SENSOR_INA700_H_ */

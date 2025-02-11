/*
 * Copyright (c) 2021  Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA23X_INA237_H_
#define ZEPHYR_DRIVERS_SENSOR_INA23X_INA237_H_

#include "ina23x_trigger.h"

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

#define INA237_REG_CONFIG     0x00
#define INA237_CFG_HIGH_PRECISION	BIT(4)

#define INA237_REG_ADC_CONFIG 0x01
#define INA237_REG_CALIB      0x02
#define INA237_REG_SHUNT_VOLT 0x04
#define INA237_REG_BUS_VOLT   0x05
#define INA237_REG_DIETEMP    0x06
#define INA237_REG_CURRENT    0x07
#define INA237_REG_POWER      0x08
#define INA237_REG_ALERT      0x0B
#define INA237_REG_SOVL       0x0C
#define INA237_REG_SUVL       0x0D
#define INA237_REG_BOVL       0x0E
#define INA237_REG_BUVL       0x0F
#define INA237_REG_TEMP_LIMIT 0x10
#define INA237_REG_PWR_LIMIT  0x11
#define INA237_REG_MANUFACTURER_ID 0x3E

#define INA237_MANUFACTURER_ID 0x5449

struct ina237_data {
	const struct device *dev;
	int16_t current;
	uint16_t bus_voltage;
	uint32_t power;
	int16_t die_temp;
#ifdef CONFIG_INA237_VSHUNT
	int16_t shunt_voltage;
#endif
	enum sensor_channel chan;
	struct ina23x_trigger trigger;
};

struct ina237_config {
	struct i2c_dt_spec bus;
	uint16_t config;
	uint16_t adc_config;
	uint32_t current_lsb;
	uint16_t cal;
	const struct gpio_dt_spec alert_gpio;
	uint16_t alert_config;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_INA23X_INA237_H_ */

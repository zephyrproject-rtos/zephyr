/*
 * Copyright 2021 Grinn
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA2XX_INA237_H_
#define ZEPHYR_DRIVERS_SENSOR_INA2XX_INA237_H_

#include "ina2xx_common.h"
#include "ina2xx_trigger.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#define INA237_DT_CONFIG(inst) DT_INST_PROP_OR(inst, high_precision, 0) << 4

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

#define INA228_REG_SHUNT_TEMPCO 0x03
#define INA228_REG_ENERGY      0x09
#define INA228_REG_CHARGE      0x0A
#define INA228_REG_DEVICE_ID   0x3F

struct ina237_data {
	struct ina2xx_data common;
	const struct device *dev;
	enum sensor_channel chan;
	struct ina2xx_trigger trigger;
};

struct ina237_config {
	const struct ina2xx_config common;
	const struct gpio_dt_spec alert_gpio;
	uint16_t alert_config;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_INA2XX_INA237_H_ */

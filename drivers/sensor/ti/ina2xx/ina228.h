/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA2XX_INA228_H_
#define ZEPHYR_DRIVERS_SENSOR_INA2XX_INA228_H_

#include "ina2xx_common.h"

#define INA228_REG_CONFIG     0x00
#define INA228_REG_ADC_CONFIG 0x01
#define INA228_REG_SHUNT_CAL  0x02
#define INA228_REG_SHUNT_VOLT 0x04
#define INA228_REG_BUS_VOLT   0x05
#define INA228_REG_DIETEMP    0x06
#define INA228_REG_CURRENT    0x07
#define INA228_REG_POWER      0x08
#define INA228_REG_ENERGY     0x09
#define INA228_REG_CHARGE     0x0A
#define INA228_REG_ALERT      0x0B
#define INA228_REG_SOVL       0x0C
#define INA228_REG_SUVL       0x0D
#define INA228_REG_BOVL       0x0E
#define INA228_REG_BUVL       0x0F
#define INA228_REG_TEMP_LIMIT 0x10
#define INA228_REG_PWR_LIMIT  0x11
#define INA228_REG_MANUFACTURER_ID 0x3E

struct ina228_data {
	const struct device *dev;
	uint64_t energy;
	uint64_t charge;
	int32_t shunt_voltage;
	int32_t bus_voltage;
	int32_t current;
	uint32_t power;
	int16_t die_temp;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_INA2XX_INA228_H_ */

/*
 * Copyright (c) 2021  Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA23X_INA237_H_
#define ZEPHYR_DRIVERS_SENSOR_INA23X_INA237_H_

#include <zephyr/drivers/i2c.h>

#define INA237_REG_CONFIG     0x00
#define INA237_REG_ADC_CONFIG 0x01
#define INA237_REG_CALIB      0x02
#define INA237_REG_SHUNT_VOLT 0x04
#define INA237_REG_BUS_VOLT   0x05
#define INA237_REG_MASK       0x06
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
	uint16_t current;
	uint16_t bus_voltage;
	uint32_t power;
};

struct ina237_config {
	struct i2c_dt_spec bus;
	uint16_t config;
	uint16_t adc_config;
	uint16_t current_lsb;
	uint16_t rshunt;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_INA23X_INA237_H_ */

/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ34110_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ34110_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bq34110, CONFIG_SENSOR_LOG_LEVEL);

#define SUB_DEVICE_TYPE		0x0001

/* Command Registers */
#define CONTROL				0x00
#define TEMPERATURE			0x06
#define VOLTAGE				0x08
#define BATTERY_STATUS			0x0a
#define CURRENT				0x0c
#define REMAINING_CAPACITY		0x10
#define FULL_CHARGE_CAPACITY		0x12
#define AVERAGE_CURRENT			0x14
#define TIME_TO_EMPTY			0x16
#define TIME_TO_FULL			0x18
#define AVERAGE_POWER			0x24
#define INTERNAL_TEMPERATURE		0x28
#define RELATIVE_STATE_OF_CHARGE	0x2c
#define STATE_OF_HEALTH			0x2e
#define MANUFACTURER_ACCESS_CONTROL	0x3e
#define MAC_DATA			0x40
#define MAC_DATA_SUM			0x60
#define MAC_DATALEN			0x61

#define TAPER_CURRENT			0x411c
#define TAPER_VOLTAGE			0x4120
#define OPERATION_CONFIG_A		0x413a
#define DESIGN_CAPACITY_MAH		0x41f5
#define DESIGN_VOLTAGE			0x41f9

/* Device ID*/
#define BQ34110_DEVICE_ID		0x0110

/* 0 °C is equal to 273.15 K */
#define ZERO_DEG_CELSIUS_IN_KELVIN	273.15

struct bq34110_data {
	uint16_t voltage;
	int16_t avg_power;
	int16_t avg_current;
	uint16_t time_to_full;
	uint16_t time_to_empty;
	int16_t state_of_health;
	uint16_t state_of_charge;
	int16_t max_load_current;
	uint16_t nom_avail_capacity;
	uint16_t full_avail_capacity;
	uint16_t internal_temperature;
	uint16_t full_charge_capacity;
	uint16_t remaining_charge_capacity;
};

struct bq34110_config {
	struct i2c_dt_spec i2c;
	uint8_t no_of_series_cells;
	uint16_t taper_current;
	uint16_t taper_voltage;
	uint16_t design_voltage;
	uint16_t design_capacity;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ34110_H_ */

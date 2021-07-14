/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ34110_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ34110_H_

#include <device.h>
#include <sys/util.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(bq34110, CONFIG_SENSOR_LOG_LEVEL);

enum {
	SUB_DEVICE_TYPE = 0x0001,
} Control_SubCommands_t;

enum {
	CONTROL = 0x00,
	TEMPERATURE = 0x06,
	VOLTAGE = 0x08,
	BATTERY_STATUS = 0x0A,
	CURRENT = 0x0C,
	REMAINING_CAPACITY = 0x10,
	FULL_CHARGE_CAPACITY = 0x12,
	AVERAGE_CURRENT = 0x14,
	TIME_TO_EMPTY = 0x16,
	TIME_TO_FULL = 0x18,
	AVERAGE_POWER = 0x24,
	INTERNAL_TEMPERATURE = 0x28,
	CMD_CYCLE_COUNT = 0x2A,
	RELATIVE_STATE_OF_CHARGE = 0x2C,
	STATE_OF_HEALTH = 0x2E,
	CHARGING_VOLTAGE = 0x30,
	CHARGING_CURRENT = 0x32,
	BLT_DISCHARGE_SET = 0x34,
	BLT_CHARGE_SET = 0x36,
	OPERATION_STATUS = 0x3A,
	DESIGN_CAPACITY = 0x3C,
	MANUFACTURER_ACCESS_CONTROL = 0x3E,
	MAC_DATA = 0x40,
	MAC_DATA_SUM = 0x60,
	MAC_DATALEN = 0x61,
} Commands_t;

enum {
	TAPER_CURRENT = 0x411C,
	TAPER_VOLTAGE = 0x4120,
} Charger_Control_t;

enum {
	OPERATION_CONFIG_A = 0x413A,
} Configuration_t;

enum {
	DESIGN_CAPACITY_MAH = 0x41F5,
	DESIGN_VOLTAGE = 0x41F9,
} GasGauging_t;

#define BQ34110_DEVICE_ID	0x0110

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
	const struct device *i2c_dev;
	uint8_t i2c_addr;
	uint8_t no_of_series_cells;
	uint16_t taper_current;
	uint16_t taper_voltage;
	uint16_t design_voltage;
	uint16_t design_capacity;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BATTERY_BQ34110_H_ */

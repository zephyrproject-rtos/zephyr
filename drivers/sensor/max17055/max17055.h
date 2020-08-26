/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_MAX17055_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_MAX17055_H_

/* Register addresses */
enum {
	REP_CAP		= 0x5,
	REP_SOC		= 0x6,
	INT_TEMP	= 0x8,
	VCELL		= 0x9,
	AVG_CURRENT	= 0xb,
	FULL_CAP_REP	= 0x10,
	TTE		= 0x11,
	CYCLES		= 0x17,
	DESIGN_CAP	= 0x18,
	TTF		= 0x20,
};

struct max17055_data {
	struct device *i2c;
	/* Current cell voltage in units of 1.25/16mV */
	uint16_t voltage;
	/* Average current in units of 1.5625uV / Rsense */
	int16_t avg_current;
	/* Remaining capacity as a %age */
	uint16_t state_of_charge;
	/* Internal temperature in units of 1/256 degrees C */
	int16_t internal_temp;
	/* Full charge capacity in 5/Rsense uA */
	uint16_t full_cap;
	/* Remaining capacity in 5/Rsense uA */
	uint16_t remaining_cap;
	/* Time to empty in seconds */
	uint16_t time_to_empty;
	/* Time to full in seconds */
	uint16_t time_to_full;
	/* Cycle count in 1/100ths (number of charge/discharge cycles) */
	uint16_t cycle_count;
	/* Design capacity in 5/Rsense uA */
	uint16_t design_cap;
};

struct max17055_config {
	char *bus_name;
	/* Value of Rsense resistor in milliohms (typicallly 5 or 10) */
	uint16_t rsense_mohms;
	/* Design voltage of cell in mV */
	uint16_t design_voltage;
	/* Desired voltage of cell in mV */
	uint16_t desired_voltage;
	/* Desired charging current in mA */
	uint16_t desired_charging_current;
};

#endif

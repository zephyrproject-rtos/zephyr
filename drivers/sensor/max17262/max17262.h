/*
 * Copyright 2021 Matija Tudan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_MAX17262_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_MAX17262_H_

#define VOLTAGE_MULTIPLIER_UV	1250 / 16
#define CURRENT_MULTIPLIER_NA	156250
#define TIME_MULTIPLIER_MS	5625

/* Register addresses */
enum {
	STATUS          = 0x00,
	REP_CAP         = 0x05,
	REP_SOC         = 0x06,
	INT_TEMP        = 0x08,
	VCELL           = 0x09,
	AVG_CURRENT     = 0x0b,
	FULL_CAP_REP    = 0x10,
	TTE             = 0x11,
	CYCLES          = 0x17,
	DESIGN_CAP      = 0x18,
	ICHG_TERM       = 0x1E,
	TTF             = 0x20,
	VEMPTY          = 0x3A,
	FSTAT           = 0x3D,
	COULOMB_COUNTER = 0x4D,
	SOFT_WAKEUP     = 0x60,
	HIBCFG          = 0xBA,
	MODELCFG        = 0xDB,
};

/* Masks */
enum {
	FSTAT_DNR        = 0x01,
	STATUS_POR       = 0x02,
	MODELCFG_REFRESH = 0x8000,
};

/* MAX17262 specific channels */
enum max17262_channel {
	MAX17262_COULOMB_COUNTER,
};

struct max17262_data {
	/* Current cell voltage in units of 1.25/16mV */
	uint16_t voltage;
	/* Average current in units of 156.25uA */
	int16_t avg_current;
	/* Desired charging current in mA */
	uint16_t ichg_term;
	/* Remaining capacity as a %age */
	uint16_t state_of_charge;
	/* Internal temperature in units of 1/256 degrees C */
	int16_t internal_temp;
	/* Full charge capacity in mAh */
	uint16_t full_cap;
	/* Remaining capacity in mAh */
	uint16_t remaining_cap;
	/* Time to empty in seconds */
	uint16_t time_to_empty;
	/* Time to full in seconds */
	uint16_t time_to_full;
	/* Cycle count in 1/100ths (number of charge/discharge cycles) */
	uint16_t cycle_count;
	/* Battery capacity in mAh */
	uint16_t design_cap;
	/* Spent capacity in mAh */
	uint16_t coulomb_counter;
};

struct max17262_config {
	const struct device *i2c;
	uint16_t i2c_addr;
	/* Value of Rsense resistor in milliohms (typically 5 or 10) */
	uint16_t rsense_mohms;
	/* Design voltage of cell in mV */
	uint16_t design_voltage;
	/* Desired voltage of cell in mV */
	uint16_t desired_voltage;
	/* Desired charging current in mA */
	uint16_t desired_charging_current;
	/* Battery capacity in mAh */
	uint16_t design_cap;
	/* Empty voltage detection in mV */
	uint16_t empty_voltage;
	/* Recovery voltage detection in mV */
	uint16_t recovery_voltage;
	/* Defined charge voltage value in mV */
	uint16_t charge_voltage;
};

#endif

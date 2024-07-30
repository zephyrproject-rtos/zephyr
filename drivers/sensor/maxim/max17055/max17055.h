/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_MAX17055_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_MAX17055_H_

#include <zephyr/drivers/i2c.h>

/* Register addresses */
enum {
	STATUS          = 0x0,
	REP_CAP         = 0x5,
	REP_SOC         = 0x6,
	INT_TEMP        = 0x8,
	VCELL           = 0x9,
	AVG_CURRENT     = 0xb,
	FULL_CAP_REP    = 0x10,
	TTE             = 0x11,
	ICHG_TERM       = 0x1e,
	CYCLES          = 0x17,
	DESIGN_CAP      = 0x18,
	TTF             = 0x20,
	V_EMPTY         = 0x3a,
	FSTAT           = 0x3d,
	D_QACC          = 0x45,
	D_PACC          = 0x46,
	SOFT_WAKEUP     = 0x60,
	HIB_CFG         = 0xba,
	MODEL_CFG       = 0xdb,
	VFOCV           = 0xfb,
};

/* Masks */
enum {
	FSTAT_DNR               = 0x0001,
	HIB_CFG_CLEAR           = 0x0000,
	MODELCFG_REFRESH        = 0x8000,
	SOFT_WAKEUP_CLEAR       = 0x0000,
	SOFT_WAKEUP_WAKEUP      = 0x0090,
	STATUS_POR              = 0x0002,
	VEMPTY_VE               = 0xff80,
};

struct max17055_data {
	/* Current cell voltage in units of 1.25/16mV */
	uint16_t voltage;
	/* Current cell open circuit voltage in units of 1.25/16mV */
	uint16_t ocv;
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
	/* Time to empty in units of 5.625s */
	uint16_t time_to_empty;
	/* Time to full in units of 5.625s */
	uint16_t time_to_full;
	/* Cycle count in 1/100ths (number of charge/discharge cycles) */
	uint16_t cycle_count;
	/* Design capacity in 5/Rsense uA */
	uint16_t design_cap;
};

struct max17055_config {
	struct i2c_dt_spec i2c;
	/* Value of Rsense resistor in milliohms (typically 5 or 10) */
	uint16_t rsense_mohms;
	/* The design capacity (aka label capacity) of the cell in mAh */
	uint16_t design_capacity;
	/* Design voltage of cell in mV */
	uint16_t design_voltage;
	/* Desired voltage of cell in mV */
	uint16_t desired_voltage;
	/* Desired charging current in mA */
	uint16_t desired_charging_current;
	/* The charge termination current in uA */
	uint16_t i_chg_term;
	/* The empty voltage of the cell in mV */
	uint16_t v_empty;
};

#endif

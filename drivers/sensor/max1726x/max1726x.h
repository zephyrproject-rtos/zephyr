/*
 * Copyright 2021 Matija Tudan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BATTERY_MAX1726X_H_
#define ZEPHYR_DRIVERS_SENSOR_BATTERY_MAX1726X_H_

#define VOLTAGE_MULTIPLIER_UV	1250 / 16
#define CURRENT_MULTIPLIER_NA	156250
#define TIME_MULTIPLIER_MS	5625

/* MAX1726X MASKS */
#define MAX1726X_HIB_ENTER_TIME_MASK	(0x07)
#define MAX1726X_HIB_THRESHOLD_MASK	(0xF)
#define MAX1726X_HIB_EXIT_TIME_MASK	(0X03)
#define MAX1726X_HIB_SCALAR_MASK	(0x07)

/* MAX1726X HIBCFG */
#define MAX1726X_EN_HIB		(BIT(15))
#define MAX1726X_HIB_ENTER_TIME(n)	((MAX1726X_HIB_ENTER_TIME_MASK & n) << 0x0C)
#define MAX1726X_HIB_THRESHOLD(n) 	((MAX1726X_HIB_THRESHOLD_MASK & n) << 0x08)
#define MAX1726X_HIB_EXIT_TIME(n)	((MAX1726X_HIB_EXIT_TIME_MASK & n) << 0x03)
#define MAX1726X_HIB_SCALAR(n)	(MAX1726X_HIB_SCALAR_MASK & n)

/* MAX1726X SHUTDOWN */
#define MAX1726X_EN_SHDN		(BIT(7))

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
	CONFIG          = 0x1D,
	SHDN_TIMER      = 0x3F,
};

/* Masks */
enum {
	FSTAT_DNR        = 0x01,
	STATUS_POR       = 0x02,
	MODELCFG_REFRESH = 0x8000,
};

/* MAX1726X specific channels */
enum max1726x_channel {
	MAX1726X_COULOMB_COUNTER,
};

struct max1726x_data {
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

struct max1726x_config {
	const struct device *i2c;
	uint16_t i2c_addr;
	/* Value of Rsense resistor in milliohms (typicallly 5 or 10) */
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
	/* Defined hibernate threshold value in mA as defined the following equation: */
	/* threshold (mA) = (FullCap(mAh)/0.8hrs)/2^(hibernate_threshold) */
	uint8_t hibernate_threshold;
	/* Defined hibernate task period in s as defined the following equation: */
	/* Task Period (s) = 351msx2^(hibernate_scalar) */
	uint8_t hibernate_scalar;
	/* Defined hibernate required time period in s of consecutive current */
	/* readings above hibernate threshold value before the IC exits */
	/* hibernate and returns to active mode using the following equation: */
	/* Exit Time (s) = (hibernate_exit_time+1)x702msx2^(hibernate_scalar) */
	uint8_t hibernate_exit_time;
	/* Defined the time period that consecutive current readings must */
	/* remain below the hibernate threshold value before the IC enters */
	/* hibernate mode, as defined by the following equation: */
	/* 2.812sx2^(hibernate_enter_time)<Entry Time<2.812sx2^(hibernate_enter_time+1) */
	uint8_t hibernate_enter_time;
};

#endif

/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BATTERY

#ifndef ZEPHYR_INCLUDE_DRIVERS_BATTERY_H_
#define ZEPHYR_INCLUDE_DRIVERS_BATTERY_H_

#include <stdbool.h>
#include <stdint.h>
#include <device.h>

/*
 * Sometimes we have hardware to detect battery present, sometimes we have to
 * wait until we've been able to talk to the battery.
 */
enum battery_present {
	BATTERY_PRESENCE_UNINITIALIZED = -1,
	BATTERY_PRESENCE_NO = 0,
	BATTERY_PRESENCE_YES = 1,
	BATTERY_PRESENCE_UNKNOWN,
};

enum battery_disconnect_state {
	BATTERY_DISCONNECTED = 0,
	BATTERY_NOT_DISCONNECTED,
	BATTERY_DISCONNECT_ERROR,
};

enum battery_property_type {
	/* Keep these alphabetized. */

	/** Runtime Dynamic Battery Parameters */
	/**
	 * Provide a 1 minute average of the current and voltage on the battery.
	 * Does not check for flags or whether those values are bad readings.
	 * See driver instance header for details on implementation and
	 * how the average is calculated. Units in mA negative=discharging
	 */
	BATTERY_AVG_CURRENT,
	BATTERY_AVG_VOLTAGE,
	/** Whether CFET is disabled, if no CFET mask given returns false */
	BATTERY_CHARGE_FET_DISABLED,
	/** Battery current (mA); negative=discharging */
	BATTERY_CURRENT,
	/** Charge cut off from the battery */
	BATTERY_CUT_OFF,
	/** Cycle count in 1/100ths (number of charge/discharge cycles) */
	BATTERY_CYCLE_COUNT,
	/** Disconnect state of battery */
	BATTERY_DISCONNECT_STATE,
	/** General Error/Runtime Flags */
	BATTERY_FLAGS,
	/** Full Charge Capacity in mAh (might change occasionally) */
	BATTERY_FULL_CHARGE_CAPACITY,
	/**
	 * Report the absolute difference between the highest and lowest cell voltage in
	 * the battery pack, in millivolts.  On error returns '0'.
	 */
	BATTERY_IMBALANCE_MV,
	/** Is the battery physically present */
	BATTERY_PRESENT_STATE,
	/** Remaining capacity in mAh */
	BATTERY_REMAINING_CAPACITY,
	/** State of charge (percent, 0-100) */
	BATTERY_STATE_OF_CHARGE,
	/** Temperature in 0.1 K */
	BATTERY_TEMPERATURE,
	/** Battery voltage (mV) */
	BATTERY_VOLTAGE,

	/** Static properties are contained in the device config */
};

#define BATTERY_INVALID_PROPERTY -ENOTSUP

struct battery_get_property {
	/** Battery property to get */
	enum battery_property_type property_type;

	/** Property field for getting */
	union {
		/** Fields have the format: */
		/** BATTERY_PROPERTY_ENUM_FIELD */
		/** type property_field; */

		/** Dynamic Battery Info */
		/** BATTERY_AVG_CURRENT, */
		int avg_current;
		/** BATTERY_AVG_VOLTAGE, */
		int avg_voltage;
		/** BATTERY_CHARGE_FET_DISABLED, */
		bool charge_fet_disabled;
		/** BATTERY_CURRENT, */
		int current;
		/** BATTERY_CUT_OFF, */
		bool cut_off;
		/** BATTERY_CYCLE_COUNT, */
		uint32_t cycle_count;
		/** BATTERY_DISCONNECT_STATE, */
		enum battery_disconnect_state disconnect_state;
		/** BATTERY_FLAGS, */
		uint32_t flags;
		/** BATTERY_FULL_CHARGE_CAPACITY, */
		uint32_t full_charge_capacity;
		/** BATTERY_IMBALANCE_MV, */
		int imbalance_mv;
		/** BATTERY_PRESENT_STATE, */
		enum battery_present battery_present_state;
		/** BATTERY_REMAINING_CAPACITY, */
		int remaining_capacity;
		/** BATTERY_RUN_TIME_TO_EMPTY, */
		uint32_t run_time_to_empty;
		/** BATTERY_STATE_OF_CHARGE, */
		uint8_t state_of_charge;
		/** BATTERY_TEMPERATURE, */
		int temperature;
		/** BATTERY_VOLTAGE, */
		int voltage;
	};
};

/**
 * Cut off charge from battery
 *
 * @return non-zero if the battery driver doesn't support cut off.
 */
typedef int (*battery_cut_off)(const struct device *dev);

/**
 * @brief Get a battery property
 *
 * @param dev Pointer to the battery device
 * @param prop pointer to battery_get_property struct where the property struct
 * field is set by the caller to determine what property is read from the
 * battery device.
 *
 * @return 0 if successful, negative errno code if failure.
 */
typedef int (*battery_get_property_t)(const struct device *dev, struct battery_get_property *prop);

/**
 * Read time in seconds left when discharging.
 *
 * @param capacity	Destination for remaining time in seconds.
 * @return non-zero if error.
 */
typedef int (*battery_time_to_empty)(const struct device *dev, int *seconds);

/**
 * Read time in seconds left to full capacity when charging.
 *
 * @param capacity	Destination for remaining time in seconds.
 * @return non-zero if error.
 */
typedef int (*battery_time_to_full)(const struct device *dev, int *seconds);

/**
 * Calculate battery time in seconds, under an assumed current.
 *
 * @param rate		Current to use for calculation, in mA.
 *			If > 0, calculates charging time; if < 0, calculates
 *			discharging time; 0 is invalid and sets seconds=0.
 * @param seconds	Destination for calculated time in seconds.
 * @return non-zero if error.
 */
typedef int (*battery_time_at_rate)(const struct device *dev, int rate, int *seconds);

/**
 * Wait for battery stable.
 *
 * @return non-zero if error.
 */
typedef int (*battery_wait_for_stable)(const struct device *dev);

/**
 * Send the fuel gauge sleep command.
 *
 * @return	0 if successful, non-zero if error occurred
 */
typedef int (*battery_sleep_fuel_gauge)(const struct device *dev);

__subsystem struct battery_driver_api {
	battery_get_property_t get_property;

	battery_time_at_rate time_at_rate;
	battery_time_to_empty time_to_empty;
	battery_time_to_full time_to_full;

	battery_wait_for_stable wait_for_stable;
	battery_sleep_fuel_gauge sleep_fuel_gauge;
	battery_cut_off cut_off_charge;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_BATTERY_H_ */

#endif /* CONFIG_BATTERY */

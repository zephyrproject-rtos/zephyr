/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BATTERY_H_
#define ZEPHYR_INCLUDE_DRIVERS_BATTERY_H_

/**
 * @brief Fuel Gauge Interface
 * @defgroup fuel_gauge_interface Fuel Gauge Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

/* Keep these alphabetized wrt property name */

/** Runtime Dynamic Battery Parameters */
/**
 * Provide a 1 minute average of the current on the battery.
 * Does not check for flags or whether those values are bad readings.
 * See driver instance header for details on implementation and
 * how the average is calculated. Units in uA negative=discharging
 */
#define FUEL_GAUGE_AVG_CURRENT 0

/** Battery current (uA); negative=discharging */
#define FUEL_GAUGE_CURRENT		FUEL_GAUGE_AVG_CURRENT + 1
/** Whether the battery underlying the fuel-gauge is cut off from charge */
#define FUEL_GAUGE_CHARGE_CUTOFF	FUEL_GAUGE_CURRENT + 1
/** Cycle count in 1/100ths (number of charge/discharge cycles) */
#define FUEL_GAUGE_CYCLE_COUNT		FUEL_GAUGE_CHARGE_CUTOFF + 1
/** Connect state of battery */
#define FUEL_GAUGE_CONNECT_STATE	FUEL_GAUGE_CYCLE_COUNT + 1
/** General Error/Runtime Flags */
#define FUEL_GAUGE_FLAGS		FUEL_GAUGE_CONNECT_STATE + 1
/** Full Charge Capacity in uAh (might change in some implementations to determine wear) */
#define FUEL_GAUGE_FULL_CHARGE_CAPACITY FUEL_GAUGE_FLAGS + 1
/** Is the battery physically present */
#define FUEL_GAUGE_PRESENT_STATE	FUEL_GAUGE_FULL_CHARGE_CAPACITY + 1
/** Remaining capacity in uAh */
#define FUEL_GAUGE_REMAINING_CAPACITY	FUEL_GAUGE_PRESENT_STATE + 1
/** Remaining battery life time in minutes */
#define FUEL_GAUGE_RUNTIME_TO_EMPTY	FUEL_GAUGE_REMAINING_CAPACITY + 1
/** Remaining time in minutes until battery reaches full charge */
#define FUEL_GAUGE_RUNTIME_TO_FULL	FUEL_GAUGE_RUNTIME_TO_EMPTY + 1
/** Retrieve word from SBS1.1 ManufactuerAccess */
#define FUEL_GAUGE_SBS_MFR_ACCESS	FUEL_GAUGE_RUNTIME_TO_FULL + 1
/** Absolute state of charge (percent, 0-100) - expressed as % of design capacity */
#define FUEL_GAUGE_STATE_OF_CHARGE	FUEL_GAUGE_SBS_MFR_ACCESS + 1
/** Temperature in 0.1 K */
#define FUEL_GAUGE_TEMPERATURE		FUEL_GAUGE_STATE_OF_CHARGE + 1
/** Battery voltage (uV) */
#define FUEL_GAUGE_VOLTAGE		FUEL_GAUGE_TEMPERATURE + 1

/** Reserved to demark end of common fuel gauge properties */
#define FUEL_GAUGE_COMMON_COUNT FUEL_GAUGE_VOLTAGE + 1
/**
 * Reserved to demark downstream custom properties - use this value as the actual value may change
 * over future versions of this API
 */
#define FUEL_GAUGE_CUSTOM_BEGIN FUEL_GAUGE_COMMON_COUNT + 1

/** Reserved to demark end of valid enum properties */
#define FUEL_GAUGE_PROP_MAX UINT16_MAX

struct fuel_gauge_get_property {
	/** Battery fuel gauge property to get */
	uint16_t property_type;

	/** Negative error status set by callee e.g. -ENOTSUP for an unsupported property */
	int status;

	/** Property field for getting */
	union {
		/* Fields have the format: */
		/* FUEL_GAUGE_PROPERTY_FIELD */
		/* type property_field; */

		/* Dynamic Battery Info */
		/** FUEL_GAUGE_AVG_CURRENT */
		int avg_current;
		/** FUEL_GAUGE_CUTOFF */
		bool cutoff;
		/** FUEL_GAUGE_CURRENT */
		int current;
		/** FUEL_GAUGE_CYCLE_COUNT */
		uint32_t cycle_count;
		/** FUEL_GAUGE_FLAGS */
		uint32_t flags;
		/** FUEL_GAUGE_FULL_CHARGE_CAPACITY */
		uint32_t full_charge_capacity;
		/** FUEL_GAUGE_REMAINING_CAPACITY */
		uint32_t remaining_capacity;
		/** FUEL_GAUGE_RUNTIME_TO_EMPTY */
		uint32_t runtime_to_empty;
		/** FUEL_GAUGE_RUNTIME_TO_FULL */
		uint32_t runtime_to_full;
		/** FUEL_GAUGE_SBS_MFR_ACCESS */
		uint16_t sbs_mfr_access_word;
		/** FUEL_GAUGE_STATE_OF_CHARGE */
		uint8_t state_of_charge;
		/** FUEL_GAUGE_TEMPERATURE */
		uint16_t temperature;
		/** FUEL_GAUGE_VOLTAGE */
		int voltage;
	} value;
};

struct fuel_gauge_set_property {
	/** Battery fuel gauge property to set */
	uint16_t property_type;

	/** Negative error status set by callee e.g. -ENOTSUP for an unsupported property */
	int status;

	/** Property field for setting */
	union {
		/* Fields have the format: */
		/* FUEL_GAUGE_PROPERTY_FIELD */
		/* type property_field; */

		/* Writable Dynamic Battery Info */
		/** FUEL_GAUGE_SBS_MFR_ACCESS */
		uint16_t sbs_mfr_access_word;
	} value;
};

/**
 * @brief Fetch a battery fuel-gauge property
 *
 * @param dev Pointer to the battery fuel-gauge device
 * @param props pointer to array of fuel_gauge_get_property struct where the property struct
 * field is set by the caller to determine what property is read from the
 * fuel gauge device into the fuel_gauge_get_property struct's value field.
 * @param props_len number of properties in props array
 *
 * @return return=0 if successful, return < 0 if getting all properties failed, return > 0 if some
 * properties failed where return=number of failing properties.
 */
typedef int (*fuel_gauge_get_property_t)(const struct device *dev,
					 struct fuel_gauge_get_property *props, size_t props_len);

/**
 * @brief Set a battery fuel-gauge property
 *
 * @param dev Pointer to the battery fuel-gauge device
 * @param props pointer to array of fuel_gauge_set_property struct where the property struct
 * field is set by the caller to determine what property is written to the fuel gauge device from
 * the fuel_gauge_get_property struct's value field.
 * @param props_len number of properties in props array
 *
 * @return return=0 if successful, return < 0 if setting all properties failed, return > 0 if some
 * properties failed where return=number of failing properties.
 */
typedef int (*fuel_gauge_set_property_t)(const struct device *dev,
					 struct fuel_gauge_set_property *props, size_t props_len);

/* Caching is entirely on the onus of the client */

__subsystem struct battery_driver_api {
	fuel_gauge_get_property_t get_property;
	fuel_gauge_set_property_t set_property;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_BATTERY_H_ */

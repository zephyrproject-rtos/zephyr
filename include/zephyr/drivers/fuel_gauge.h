/*
 * Copyright 2022 Google LLC
 * Copyright 2023 Microsoft Corporation
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

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

/* Keep these alphabetized wrt property name */

enum fuel_gauge_property {
	/** Runtime Dynamic Battery Parameters */
	/**
	 * Provide a 1 minute average of the current on the battery.
	 * Does not check for flags or whether those values are bad readings.
	 * See driver instance header for details on implementation and
	 * how the average is calculated. Units in uA negative=discharging
	 */
	FUEL_GAUGE_AVG_CURRENT = 0,

	/** Battery current (uA); negative=discharging */
	FUEL_GAUGE_CURRENT,
	/** Whether the battery underlying the fuel-gauge is cut off from charge */
	FUEL_GAUGE_CHARGE_CUTOFF,
	/** Cycle count in 1/100ths (number of charge/discharge cycles) */
	FUEL_GAUGE_CYCLE_COUNT,
	/** Connect state of battery */
	FUEL_GAUGE_CONNECT_STATE,
	/** General Error/Runtime Flags */
	FUEL_GAUGE_FLAGS,
	/** Full Charge Capacity in uAh (might change in some implementations to determine wear) */
	FUEL_GAUGE_FULL_CHARGE_CAPACITY,
	/** Is the battery physically present */
	FUEL_GAUGE_PRESENT_STATE,
	/** Remaining capacity in uAh */
	FUEL_GAUGE_REMAINING_CAPACITY,
	/** Remaining battery life time in minutes */
	FUEL_GAUGE_RUNTIME_TO_EMPTY,
	/** Remaining time in minutes until battery reaches full charge */
	FUEL_GAUGE_RUNTIME_TO_FULL,
	/** Retrieve word from SBS1.1 ManufactuerAccess */
	FUEL_GAUGE_SBS_MFR_ACCESS,
	/** Absolute state of charge (percent, 0-100) - expressed as % of design capacity */
	FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE,
	/** Relative state of charge (percent, 0-100) - expressed as % of full charge capacity */
	FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE,
	/** Temperature in 0.1 K */
	FUEL_GAUGE_TEMPERATURE,
	/** Battery voltage (uV) */
	FUEL_GAUGE_VOLTAGE,
	/** Battery Mode (flags) */
	FUEL_GAUGE_SBS_MODE,
	/** Battery desired Max Charging Current (mA) */
	FUEL_GAUGE_CHARGE_CURRENT,
	/** Battery desired Max Charging Voltage (mV) */
	FUEL_GAUGE_CHARGE_VOLTAGE,
	/** Alarm, Status and Error codes (flags) */
	FUEL_GAUGE_STATUS,
	/** Design Capacity (mAh or 10mWh) */
	FUEL_GAUGE_DESIGN_CAPACITY,
	/** Design Voltage (mV) */
	FUEL_GAUGE_DESIGN_VOLTAGE,
	/** AtRate (mA or 10 mW) */
	FUEL_GAUGE_SBS_ATRATE,
	/** AtRateTimeToFull (minutes) */
	FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL,
	/** AtRateTimeToEmpty (minutes) */
	FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY,
	/** AtRateOK (boolean) */
	FUEL_GAUGE_SBS_ATRATE_OK,
	/** Remaining Capacity Alarm (mAh or 10mWh) */
	FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM,
	/** Remaining Time Alarm (minutes) */
	FUEL_GAUGE_SBS_REMAINING_TIME_ALARM,
	/** Manufacturer of pack (1 byte length + 20 bytes data) */
	FUEL_GAUGE_MANUFACTURER_NAME,
	/** Name of pack (1 byte length + 20 bytes data) */
	FUEL_GAUGE_DEVICE_NAME,
	/** Chemistry (1 byte length + 4 bytes data) */
	FUEL_GAUGE_DEVICE_CHEMISTRY,

	/** Reserved to demark end of common fuel gauge properties */
	FUEL_GAUGE_COMMON_COUNT,
	/**
	 * Reserved to demark downstream custom properties - use this value as the actual value may
	 * change over future versions of this API
	 */
	FUEL_GAUGE_CUSTOM_BEGIN,

	/** Reserved to demark end of valid enum properties */
	FUEL_GAUGE_PROP_MAX = UINT16_MAX,
};

typedef uint16_t fuel_gauge_prop_t;

struct fuel_gauge_get_property {
	/** Battery fuel gauge property to get */
	fuel_gauge_prop_t property_type;

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
		/** FUEL_GAUGE_CHARGE_CUTOFF */
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
		/** FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE */
		uint8_t absolute_state_of_charge;
		/** FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE */
		uint8_t relative_state_of_charge;
		/** FUEL_GAUGE_TEMPERATURE */
		uint16_t temperature;
		/** FUEL_GAUGE_VOLTAGE */
		int voltage;
		/** FUEL_GAUGE_SBS_MODE */
		uint16_t sbs_mode;
		/** FUEL_GAUGE_CHARGE_CURRENT */
		uint16_t chg_current;
		/** FUEL_GAUGE_CHARGE_VOLTAGE */
		uint16_t chg_voltage;
		/** FUEL_GAUGE_STATUS */
		uint16_t fg_status;
		/** FUEL_GAUGE_DESIGN_CAPACITY */
		uint16_t design_cap;
		/** FUEL_GAUGE_DESIGN_VOLTAGE */
		uint16_t design_volt;
		/** FUEL_GAUGE_SBS_ATRATE */
		int16_t sbs_at_rate;
		/** FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL */
		uint16_t sbs_at_rate_time_to_full;
		/** FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY	*/
		uint16_t sbs_at_rate_time_to_empty;
		/** FUEL_GAUGE_SBS_ATRATE_OK */
		bool sbs_at_rate_ok;
		/** FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM */
		uint16_t sbs_remaining_capacity_alarm;
		/** FUEL_GAUGE_SBS_REMAINING_TIME_ALARM */
		uint16_t sbs_remaining_time_alarm;
	} value;
};

struct fuel_gauge_set_property {
	/** Battery fuel gauge property to set */
	fuel_gauge_prop_t property_type;

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
		/** FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM */
		uint16_t sbs_remaining_capacity_alarm;
		/** FUEL_GAUGE_SBS_REMAINING_TIME_ALARM */
		uint16_t sbs_remaining_time_alarm;
		/** FUEL_GAUGE_SBS_MODE */
		uint16_t sbs_mode;
		/** FUEL_GAUGE_SBS_ATRATE */
		int16_t sbs_at_rate;
	} value;
};

/** Buffer properties are separated due to size */
struct fuel_gauge_get_buffer_property {
	/** Battery fuel gauge property to get */
	fuel_gauge_prop_t property_type;

	/** Negative error status set by callee e.g. -ENOTSUP for an unsupported property */
	int status;
};

/**
 * Data structures for reading SBS buffer properties
 */
#define SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE 20
#define SBS_GAUGE_DEVICE_NAME_MAX_SIZE       20
#define SBS_GAUGE_DEVICE_CHEMISTRY_MAX_SIZE  4

struct sbs_gauge_manufacturer_name {
	uint8_t manufacturer_name_length;
	char manufacturer_name[SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE];
} __packed;

struct sbs_gauge_device_name {
	uint8_t device_name_length;
	char device_name[SBS_GAUGE_DEVICE_NAME_MAX_SIZE];
} __packed;

struct sbs_gauge_device_chemistry {
	uint8_t device_chemistry_length;
	char device_chemistry[SBS_GAUGE_DEVICE_CHEMISTRY_MAX_SIZE];
} __packed;

/**
 * @typedef fuel_gauge_get_property_t
 * @brief Callback API for getting a fuel_gauge property.
 *
 * See fuel_gauge_get_property() for argument description
 */
typedef int (*fuel_gauge_get_property_t)(const struct device *dev,
					 struct fuel_gauge_get_property *props, size_t props_len);

/**
 * @typedef fuel_gauge_set_property_t
 * @brief Callback API for setting a fuel_gauge property.
 *
 * See fuel_gauge_set_property() for argument description
 */
typedef int (*fuel_gauge_set_property_t)(const struct device *dev,
					 struct fuel_gauge_set_property *props, size_t props_len);

/**
 * @typedef fuel_gauge_get_buffer_property_t
 * @brief Callback API for getting a fuel_gauge buffer property.
 *
 * See fuel_gauge_get_buffer_property() for argument description
 */
typedef int (*fuel_gauge_get_buffer_property_t)(const struct device *dev,
					       struct fuel_gauge_get_buffer_property *prop,
					       void *dst, size_t dst_len);


/* Caching is entirely on the onus of the client */

__subsystem struct fuel_gauge_driver_api {
	fuel_gauge_get_property_t get_property;
	fuel_gauge_set_property_t set_property;
	fuel_gauge_get_buffer_property_t get_buffer_property;
};

/**
 * @brief Fetch a battery fuel-gauge property
 *
 * @param dev Pointer to the battery fuel-gauge device
 * @param props pointer to array of fuel_gauge_get_property struct where the property struct
 * field is set by the caller to determine what property is read from the
 * fuel gauge device into the fuel_gauge_get_property struct's value field. The props array
 * maintains the same order of properties as it was given.
 * @param props_len number of properties in props array
 *
 * @return return=0 if successful, return < 0 if getting all properties failed, return > 0 if some
 * properties failed where return=number of failing properties.
 */
__syscall int fuel_gauge_get_prop(const struct device *dev, struct fuel_gauge_get_property *props,
				  size_t props_len);

static inline int z_impl_fuel_gauge_get_prop(const struct device *dev,
					     struct fuel_gauge_get_property *props,
					     size_t props_len)
{
	const struct fuel_gauge_driver_api *api = (const struct fuel_gauge_driver_api *)dev->api;

	if (api->get_property == NULL) {
		return -ENOSYS;
	}

	return api->get_property(dev, props, props_len);
}

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
__syscall int fuel_gauge_set_prop(const struct device *dev, struct fuel_gauge_set_property *props,
				  size_t props_len);

static inline int z_impl_fuel_gauge_set_prop(const struct device *dev,
					     struct fuel_gauge_set_property *props,
					     size_t props_len)
{
	const struct fuel_gauge_driver_api *api = (const struct fuel_gauge_driver_api *)dev->api;

	if (api->set_property == NULL) {
		return -ENOSYS;
	}

	return api->set_property(dev, props, props_len);
}

/**
 * @brief Fetch a battery fuel-gauge buffer property
 *
 * @param dev Pointer to the battery fuel-gauge device
 * @param prop pointer to single fuel_gauge_get_buffer_property struct where the property struct
 * field is set by the caller to determine what property is read from the
 * fuel gauge device into the dst field.
 * @param dst byte array or struct that will hold the buffer data that is read from the fuel gauge
 * @param dst_len the length of the destination array in bytes
 *
 * @return return=0 if successful, return < 0 if getting property failed, return 0 on success
 */
__syscall int fuel_gauge_get_buffer_prop(const struct device *dev,
					struct fuel_gauge_get_buffer_property *prop, void *dst,
					size_t dst_len);

static inline int z_impl_fuel_gauge_get_buffer_prop(const struct device *dev,
						   struct fuel_gauge_get_buffer_property *prop,
						   void *dst, size_t dst_len)
{
	const struct fuel_gauge_driver_api *api = (const struct fuel_gauge_driver_api *)dev->api;

	if (api->get_buffer_property == NULL) {
		return -ENOSYS;
	}

	return api->get_buffer_property(dev, prop, dst, dst_len);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <syscalls/fuel_gauge.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_BATTERY_H_ */

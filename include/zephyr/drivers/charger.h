/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Charger APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CHARGER_H_
#define ZEPHYR_INCLUDE_DRIVERS_CHARGER_H_

/**
 * @brief Charger Interface
 * @defgroup charger_interface Charger Interface
 * @ingroup io_interfaces
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Runtime Dynamic Battery Parameters
 */
enum charger_property {
	/** Indicates if external supply is present for the charger. */
	/** Value should be of type enum charger_online */
	CHARGER_PROP_ONLINE = 0,
	/** Reports whether or not a battery is present. */
	/** Value should be of type bool*/
	CHARGER_PROP_PRESENT,
	/** Represents the charging status of the charger. */
	/** Value should be of type enum charger_status */
	CHARGER_PROP_STATUS,
	/** Represents the charging algo type of the charger. */
	/** Value should be of type enum charger_charge_type */
	CHARGER_PROP_CHARGE_TYPE,
	/** Represents the health of the charger. */
	/** Value should be of type enum charger_health */
	CHARGER_PROP_HEALTH,
	/** Configuration of current sink used for charging in µA */
	CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA,
	/** Configuration of current sink used for conditioning in µA */
	CHARGER_PROP_PRECHARGE_CURRENT_UA,
	/** Configuration of charge termination target in µA */
	CHARGER_PROP_CHARGE_TERM_CURRENT_UA,
	/** Configuration of charge voltage regulation target in µV */
	CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV,
	/**
	 * Configuration of the input current regulation target in µA
	 *
	 * This value is a rising current threshold that is regulated by reducing the charge
	 * current output
	 */
	CHARGER_PROP_INPUT_REGULATION_CURRENT_UA,
	/**
	 * Configuration of the input voltage regulation target in µV
	 *
	 * This value is a falling voltage threshold that is regulated by reducing the charge
	 * current output
	 */
	CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV,
	/**
	 * Configuration to issue a notification to the system based on the input current
	 * level and timing
	 *
	 * Value should be of type struct charger_current_notifier
	 */
	CHARGER_PROP_INPUT_CURRENT_NOTIFICATION,
	/**
	 * Configuration to issue a notification to the system based on the battery discharge
	 * current level and timing
	 *
	 * Value should be of type struct charger_current_notifier
	 */
	CHARGER_PROP_DISCHARGE_CURRENT_NOTIFICATION,
	/**
	 * Configuration of the falling system voltage threshold where a notification
	 * is issued to the system, measured in µV
	 */
	CHARGER_PROP_SYSTEM_VOLTAGE_NOTIFICATION_UV,
	/** Reserved to demark end of common charger properties */
	CHARGER_PROP_COMMON_COUNT,
	/**
	 * Reserved to demark downstream custom properties - use this value as the actual value may
	 * change over future versions of this API
	 */
	CHARGER_PROP_CUSTOM_BEGIN = CHARGER_PROP_COMMON_COUNT + 1,
	/** Reserved to demark end of valid enum properties */
	CHARGER_PROP_MAX = UINT16_MAX,
};

/**
 * @typedef charger_prop_t
 * @brief A charger property's identifier
 *
 * See charger_property for a list of identifiers
 */
typedef uint16_t charger_prop_t;

/**
 * @brief External supply states
 */
enum charger_online {
	/** External supply not present */
	CHARGER_ONLINE_OFFLINE = 0,
	/** External supply is present and of fixed output */
	CHARGER_ONLINE_FIXED,
	/** External supply is present and of programmable output*/
	CHARGER_ONLINE_PROGRAMMABLE,
};

/**
 * @brief Charging states
 */
enum charger_status {
	/** Charging device state is unknown */
	CHARGER_STATUS_UNKNOWN = 0,
	/** Charging device is charging a battery */
	CHARGER_STATUS_CHARGING,
	/** Charging device is not able to charge a battery */
	CHARGER_STATUS_DISCHARGING,
	/** Charging device is not charging a battery */
	CHARGER_STATUS_NOT_CHARGING,
	/** The battery is full and the charging device will not attempt charging */
	CHARGER_STATUS_FULL,
};

/**
 * @brief Charge algorithm types
 */
enum charger_charge_type {
	/** Charge type is unknown */
	CHARGER_CHARGE_TYPE_UNKNOWN = 0,
	/** Charging is not occurring */
	CHARGER_CHARGE_TYPE_NONE,
	/**
	 * Charging is occurring at the slowest desired charge rate,
	 * typically for battery detection or preconditioning
	 */
	CHARGER_CHARGE_TYPE_TRICKLE,
	/** Charging is occurring at the fastest desired charge rate */
	CHARGER_CHARGE_TYPE_FAST,
	/** Charging is occurring at a moderate charge rate */
	CHARGER_CHARGE_TYPE_STANDARD,
	/*
	 * Charging is being dynamically adjusted by the charger device
	 */
	CHARGER_CHARGE_TYPE_ADAPTIVE,
	/*
	 * Charging is occurring at a reduced charge rate to preserve
	 * battery health
	 */
	CHARGER_CHARGE_TYPE_LONGLIFE,
	/*
	 * The charger device is being bypassed and the power conversion
	 * is being handled externally, typically by a "smart" wall adaptor
	 */
	CHARGER_CHARGE_TYPE_BYPASS,
};

/**
 * @brief Charger health conditions
 *
 * These conditions determine the ability to, or the rate of, charge
 */
enum charger_health {
	/** Charger health condition is unknown */
	CHARGER_HEALTH_UNKNOWN = 0,
	/** Charger health condition is good */
	CHARGER_HEALTH_GOOD,
	/** The charger device is overheated */
	CHARGER_HEALTH_OVERHEAT,
	/** The battery voltage has exceeded its overvoltage threshold */
	CHARGER_HEALTH_OVERVOLTAGE,
	/**
	 * The battery or charger device is experiencing an unspecified
	 * failure.
	 */
	CHARGER_HEALTH_UNSPEC_FAILURE,
	/** The battery temperature is below the "cold" threshold */
	CHARGER_HEALTH_COLD,
	/** The charger device's watchdog timer has expired */
	CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE,
	/** The charger device's safety timer has expired */
	CHARGER_HEALTH_SAFETY_TIMER_EXPIRE,
	/** The charger device requires calibration */
	CHARGER_HEALTH_CALIBRATION_REQUIRED,
	/** The battery temperature is in the "warm" range */
	CHARGER_HEALTH_WARM,
	/** The battery temperature is in the "cool" range */
	CHARGER_HEALTH_COOL,
	/** The battery temperature is below the "hot" threshold */
	CHARGER_HEALTH_HOT,
	/** The charger device does not detect a battery */
	CHARGER_HEALTH_NO_BATTERY,
};

/**
 * @brief Charger severity levels for system notifications
 */
enum charger_notification_severity {
	/** Most severe level, typically triggered instantaneously */
	CHARGER_SEVERITY_PEAK = 0,
	/** More severe than the warning level, less severe than peak */
	CHARGER_SEVERITY_CRITICAL,
	/** Base severity level */
	CHARGER_SEVERITY_WARNING,
};

/**
 * @brief The input current thresholds for the charger to notify the system
 */
struct charger_current_notifier {
	/** The severity of the notification where CHARGER_SEVERITY_PEAK is the most severe */
	uint8_t severity;
	/** The current threshold to be exceeded */
	uint32_t current_ua;
	/** The duration of excess current before notifying the system */
	uint32_t duration_us;
};

/**
 * @brief container for a charger_property value
 *
 */
union charger_propval {
	/* Fields have the format: */
	/* CHARGER_PROPERTY_FIELD */
	/* type property_field; */

	/** CHARGER_PROP_ONLINE */
	enum charger_online online;
	/** CHARGER_PROP_PRESENT */
	bool present;
	/** CHARGER_PROP_STATUS */
	enum charger_status status;
	/** CHARGER_PROP_CHARGE_TYPE */
	enum charger_charge_type charge_type;
	/** CHARGER_PROP_HEALTH */
	enum charger_health health;
	/** CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA */
	uint32_t const_charge_current_ua;
	/** CHARGER_PROP_PRECHARGE_CURRENT_UA */
	uint32_t precharge_current_ua;
	/** CHARGER_PROP_CHARGE_TERM_CURRENT_UA */
	uint32_t charge_term_current_ua;
	/** CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV */
	uint32_t const_charge_voltage_uv;
	/** CHARGER_PROP_INPUT_REGULATION_CURRENT_UA */
	uint32_t input_current_regulation_current_ua;
	/** CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV */
	uint32_t input_voltage_regulation_voltage_uv;
	/** CHARGER_PROP_INPUT_CURRENT_NOTIFICATION */
	struct charger_current_notifier input_current_notification;
	/** CHARGER_PROP_DISCHARGE_CURRENT_NOTIFICATION */
	struct charger_current_notifier discharge_current_notification;
	/** CHARGER_PROP_SYSTEM_VOLTAGE_NOTIFICATION_UV */
	uint32_t system_voltage_notification;
};

/**
 * @typedef charger_get_property_t
 * @brief Callback API for getting a charger property.
 *
 * See charger_get_property() for argument description
 */
typedef int (*charger_get_property_t)(const struct device *dev, const charger_prop_t prop,
				      union charger_propval *val);

/**
 * @typedef charger_set_property_t
 * @brief Callback API for setting a charger property.
 *
 * See charger_set_property() for argument description
 */
typedef int (*charger_set_property_t)(const struct device *dev, const charger_prop_t prop,
				      const union charger_propval *val);

/**
 * @typedef charger_charge_enable_t
 * @brief Callback API enabling or disabling a charge cycle.
 *
 * See charger_charge_enable() for argument description
 */
typedef int (*charger_charge_enable_t)(const struct device *dev, const bool enable);

/**
 * @brief Charging device API
 *
 * Caching is entirely on the onus of the client
 */
__subsystem struct charger_driver_api {
	charger_get_property_t get_property;
	charger_set_property_t set_property;
	charger_charge_enable_t charge_enable;
};

/**
 * @brief Fetch a battery charger property
 *
 * @param dev Pointer to the battery charger device
 * @param prop Charger property to get
 * @param val Pointer to charger_propval union
 *
 * @retval 0 if successful
 * @retval < 0 if getting property failed
 */
__syscall int charger_get_prop(const struct device *dev, const charger_prop_t prop,
			       union charger_propval *val);

static inline int z_impl_charger_get_prop(const struct device *dev, const charger_prop_t prop,
					  union charger_propval *val)
{
	const struct charger_driver_api *api = (const struct charger_driver_api *)dev->api;

	return api->get_property(dev, prop, val);
}

/**
 * @brief Set a battery charger property
 *
 * @param dev Pointer to the battery charger device
 * @param prop Charger property to set
 * @param val Pointer to charger_propval union
 *
 * @retval 0 if successful
 * @retval < 0 if setting property failed
 */
__syscall int charger_set_prop(const struct device *dev, const charger_prop_t prop,
			       const union charger_propval *val);

static inline int z_impl_charger_set_prop(const struct device *dev, const charger_prop_t prop,
					  const union charger_propval *val)
{
	const struct charger_driver_api *api = (const struct charger_driver_api *)dev->api;

	return api->set_property(dev, prop, val);
}

/**
 * @brief Enable or disable a charge cycle
 *
 * @param dev Pointer to the battery charger device
 * @param enable true enables a charge cycle, false disables a charge cycle
 *
 * @retval 0 if successful
 * @retval -EIO if communication with the charger failed
 * @retval -EINVAL if the conditions for initiating charging are invalid
 */
__syscall int charger_charge_enable(const struct device *dev, const bool enable);

static inline int z_impl_charger_charge_enable(const struct device *dev, const bool enable)
{
	const struct charger_driver_api *api = (const struct charger_driver_api *)dev->api;

	return api->charge_enable(dev, enable);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <syscalls/charger.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CHARGER_H_ */

/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
 * @brief Charging device API
 *
 * Caching is entirely on the onus of the client
 */
__subsystem struct charger_driver_api {
	charger_get_property_t get_property;
	charger_set_property_t set_property;
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
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <syscalls/charger.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CHARGER_H_ */

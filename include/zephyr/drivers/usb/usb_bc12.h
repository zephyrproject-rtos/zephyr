/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the USB BC1.2 battery charging detect drivers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USB_USB_BC12_H_
#define ZEPHYR_INCLUDE_DRIVERS_USB_USB_BC12_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BC1.2 driver APIs
 * @defgroup b12_interface BC1.2 driver APIs
 * @ingroup io_interfaces
 * @{
 */

/* FIXME - make these Kconfig options */

/**
 * @name BC1.2 constants
 * @{
 */

/** BC1.2 USB charger voltage. */
#define BC12_CHARGER_VOLTAGE_UV	 5000 * 1000
/**
 * BC1.2 USB charger minimum current. Set to match the Isusp of 2.5 mA parameter.
 * This is returned by the driver when either BC1.2 detection fails, or the
 * attached partner is a SDP (standard downstream port).
 *
 * The application may increase the current draw after determing the USB device
 * state of suspended/unconfigured/configured.
 *   Suspended: 2.5 mA
 *   Unconfigured: 100 mA
 *   Configured: 500 mA (USB 2.0)
 */
#define BC12_CHARGER_MIN_CURR_UA 2500
/** BC1.2 USB charger maximum current. */
#define BC12_CHARGER_MAX_CURR_UA 1500 * 1000

/** @} */

/** @cond INTERNAL_HIDDEN
 * @brief Helper macro for setting a BC1.2 current limit
 *
 * @param val Current limit value, in uA.
 * @return A valid BC1.2 current limit, in uA, clamped between the BC1.2 minimum
 * and maximum values.
 */
#define BC12_CURR_UA(val) CLAMP(val, BC12_CHARGER_MIN_CURR_UA, BC12_CHARGER_MAX_CURR_UA)
/** @endcond  */

/** @brief BC1.2 device role. */
enum bc12_role {
	BC12_DISCONNECTED,
	BC12_PORTABLE_DEVICE,
	BC12_CHARGING_PORT,
};

/** @brief BC1.2 charging partner type. */
enum bc12_type {
	/**  No partner connected. */
	BC12_TYPE_NONE,
	/** Standard Downstream Port */
	BC12_TYPE_SDP,
	/** Dedicated Charging Port */
	BC12_TYPE_DCP,
	/** Charging Downstream Port */
	BC12_TYPE_CDP,
	/** Proprietary charging port */
	BC12_TYPE_PROPRIETARY,
	/** Unknown charging port, BC1.2 detection failed. */
	BC12_TYPE_UNKNOWN,
	/** Count of valid BC12 types. */
	BC12_TYPE_COUNT,
};

/**
 * @brief BC1.2 detected partner state.
 *
 * @param bc12_role Current role of the BC1.2 device.
 * @param type Charging partner type. Valid when bc12_role is BC12_PORTABLE_DEVICE.
 * @param current_ma Current, in uA, that the charging partner provides. Valid when bc12_role is
 * BC12_PORTABLE_DEVICE.
 * @param voltage_mv Voltage, in uV, that the charging partner provides. Valid when bc12_role is
 * BC12_PORTABLE_DEVICE.
 * @param pd_partner_connected True if a PD partner is currently connected. Valid when bc12_role is
 * BC12_CHARGING_PORT.
 */
struct bc12_partner_state {
	enum bc12_role bc12_role;
	union {
		struct {
			enum bc12_type type;
			int current_ua;
			int voltage_uv;
		};
		struct {
			bool pd_partner_connected;
		};
	};
};

/**
 * @brief BC1.2 callback for charger configuration
 *
 * @param dev BC1.2 device which is notifying of the new charger state.
 * @param state Current state of the BC1.2 client, including BC1.2 type
 * detected, voltage, and current limits.
 * If NULL, then the partner charger is disconnected or the BC1.2 device is
 * operating in host mode.
 * @param user_data Requester supplied data which is passed along to the callback.
 */
typedef void (*bc12_callback_t)(const struct device *dev, struct bc12_partner_state *state,
				void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
 */
__subsystem struct bc12_driver_api {
	int (*set_role)(const struct device *dev, enum bc12_role role);
	int (*set_result_cb)(const struct device *dev, bc12_callback_t cb, void *user_data);
};
/**
 * @endcond
 */

/**
 * @brief Set the BC1.2 role.
 *
 * @param dev Pointer to the device structure for the BC1.2 driver instance.
 * @param role New role for the BC1.2 device.
 *
 * @retval 0 If successful.
 * @retval -EIO general input/output error.
 */
__syscall int bc12_set_role(const struct device *dev, enum bc12_role role);

static inline int z_impl_bc12_set_role(const struct device *dev, enum bc12_role role)
{
	const struct bc12_driver_api *api = (const struct bc12_driver_api *)dev->api;

	return api->set_role(dev, role);
}

/**
 * @brief Register a callback for BC1.2 results.
 *
 * @param dev Pointer to the device structure for the BC1.2 driver instance.
 * @param cb Function pointer for the result callback.
 * @param user_data Requester supplied data which is passed along to the callback.
 *
 * @retval 0 If successful.
 * @retval -EIO general input/output error.
 */
__syscall int bc12_set_result_cb(const struct device *dev, bc12_callback_t cb, void *user_data);

static inline int z_impl_bc12_set_result_cb(const struct device *dev, bc12_callback_t cb,
					    void *user_data)
{
	const struct bc12_driver_api *api = (const struct bc12_driver_api *)dev->api;

	return api->set_result_cb(dev, cb, user_data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/usb_bc12.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_USB_USB_BC12_H_ */

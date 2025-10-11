/*
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Type-C Power Path Controller device API
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_PPC_H_
#define ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_PPC_H_

/**
 * @brief USB Type-C Power Path Controller
 * @defgroup usb_type_c_power_path_controller USB Type-C Power Path Controller
 * @ingroup usb_type_c
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Type of event being notified by Power Path Controller */
enum usbc_ppc_event {
	/** Exit from dead-battery mode failed */
	USBC_PPC_EVENT_DEAD_BATTERY_ERROR = 0,

	/** Overvoltage detected while being in a source role */
	USBC_PPC_EVENT_SRC_OVERVOLTAGE,
	/** Reverse current detected while being in a source role */
	USBC_PPC_EVENT_SRC_REVERSE_CURRENT,
	/** Overcurrent detected while being in a source role */
	USBC_PPC_EVENT_SRC_OVERCURRENT,
	/** VBUS short detected while being in a source role */
	USBC_PPC_EVENT_SRC_SHORT,

	/** Chip over temperature detected  */
	USBC_PPC_EVENT_OVER_TEMPERATURE,
	/** Sink and source paths enabled simultaneously */
	USBC_PPC_EVENT_BOTH_SNKSRC_ENABLED,

	/** Reverse current detected while being in a sink role */
	USBC_PPC_EVENT_SNK_REVERSE_CURRENT,
	/** VBUS short detected while being in a sink role */
	USBC_PPC_EVENT_SNK_SHORT,
	/** Overvoltage detected while being in a sink role */
	USBC_PPC_EVENT_SNK_OVERVOLTAGE,
};

typedef void (*usbc_ppc_event_cb_t)(const struct device *dev, void *data, enum usbc_ppc_event ev);

/** Structure with pointers to the functions implemented by driver */
__subsystem struct usbc_ppc_driver_api {
	int (*is_dead_battery_mode)(const struct device *dev);
	int (*exit_dead_battery_mode)(const struct device *dev);
	int (*is_vbus_source)(const struct device *dev);
	int (*is_vbus_sink)(const struct device *dev);
	int (*set_snk_ctrl)(const struct device *dev, bool enable);
	int (*set_src_ctrl)(const struct device *dev, bool enable);
	int (*set_vbus_discharge)(const struct device *dev, bool enable);
	int (*is_vbus_present)(const struct device *dev);
	int (*set_event_handler)(const struct device *dev, usbc_ppc_event_cb_t handler, void *data);
	int (*dump_regs)(const struct device *dev);
};

/*
 * API functions
 */

/**
 * @brief Check if PPC is in the dead battery mode
 *
 * @param dev PPC device structure
 * @retval 1 if PPC is in the dead battery mode
 * @retval 0 if PPC is not in the dead battery mode
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_is_dead_battery_mode(const struct device *dev)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->is_dead_battery_mode == NULL) {
		return -ENOSYS;
	}

	return api->is_dead_battery_mode(dev);
}

/**
 * @brief Request the PPC to exit from the dead battery mode
 * Return from this call doesn't mean that the PPC is not in the dead battery anymore.
 * In the case of error, the driver should execute the callback with
 * USBC_PPC_EVENT_DEAD_BATTERY_ERROR enum. To check if the PPC disabled the dead battery mode,
 * the call to ppc_is_dead_battery_mode should be done.
 *
 * @param dev PPC device structure
 * @retval 0 if request was successfully sent
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_exit_dead_battery_mode(const struct device *dev)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->exit_dead_battery_mode == NULL) {
		return -ENOSYS;
	}

	return api->exit_dead_battery_mode(dev);
}

/**
 * @brief Check if the PPC is sourcing the VBUS
 *
 * @param dev PPC device structure
 * @retval 1 if the PPC is sourcing the VBUS
 * @retval 0 if the PPC is not sourcing the VBUS
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_is_vbus_source(const struct device *dev)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->is_vbus_source == NULL) {
		return -ENOSYS;
	}

	return api->is_vbus_source(dev);
}

/**
 * @brief Check if the PPC is sinking the VBUS
 *
 * @param dev PPC device structure
 * @retval 1 if the PPC is sinking the VBUS
 * @retval 0 if the PPC is not sinking the VBUS
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_is_vbus_sink(const struct device *dev)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->is_vbus_sink == NULL) {
		return -ENOSYS;
	}

	return api->is_vbus_sink(dev);
}

/**
 * @brief Set the state of VBUS sinking
 *
 * @param dev PPC device structure
 * @param enable True if sinking VBUS should be enabled, false if should be disabled
 * @retval 0 if success
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_set_snk_ctrl(const struct device *dev, bool enable)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->set_snk_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->set_snk_ctrl(dev, enable);
}

/**
 * @brief Set the state of VBUS sourcing
 *
 * @param dev PPC device structure
 * @param enable True if sourcing VBUS should be enabled, false if should be disabled
 * @retval 0 if success
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_set_src_ctrl(const struct device *dev, bool enable)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->set_src_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->set_src_ctrl(dev, enable);
}

/**
 * @brief Set the state of VBUS discharging
 *
 * @param dev PPC device structure
 * @param enable True if VBUS discharging should be enabled, false if should be disabled
 * @retval 0 if success
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_set_vbus_discharge(const struct device *dev, bool enable)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->set_vbus_discharge == NULL) {
		return -ENOSYS;
	}

	return api->set_vbus_discharge(dev, enable);
}

/**
 * @brief Check if VBUS is present
 *
 * @param dev PPC device structure
 * @retval 1 if VBUS voltage is present
 * @retval 0 if no VBUS voltage is detected
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_is_vbus_present(const struct device *dev)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->is_vbus_present == NULL) {
		return -ENOSYS;
	}

	return api->is_vbus_present(dev);
}

/**
 * @brief Set the callback used to notify about PPC events
 *
 * @param dev PPC device structure
 * @param handler Handler that will be called with events notifications
 * @param data Pointer used as an argument to the callback
 * @retval 0 if success
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_set_event_handler(const struct device *dev,
	usbc_ppc_event_cb_t handler, void *data)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->set_event_handler == NULL) {
		return -ENOSYS;
	}

	return api->set_event_handler(dev, handler, data);
}

/**
 * @brief Print the values or PPC registers
 *
 * @param dev PPC device structure
 * @retval 0 if success
 * @retval -EIO if I2C communication failed
 * @retval -ENOSYS if this function is not supported by the driver
 */
static inline int ppc_dump_regs(const struct device *dev)
{
	const struct usbc_ppc_driver_api *api = (const struct usbc_ppc_driver_api *)dev->api;

	if (api->dump_regs == NULL) {
		return -ENOSYS;
	}

	return api->dump_regs(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_PPC_H_ */

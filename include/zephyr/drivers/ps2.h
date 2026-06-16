/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup ps2_interface
 * @brief Main header file for PS/2 (Personal System/2) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PS2_H_
#define ZEPHYR_INCLUDE_DRIVERS_PS2_H_

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for PS/2 devices.
 * @defgroup ps2_interface PS/2
 * @ingroup io_interfaces
 *
 * Callers of this API are responsible for setting the typematic rate
 * and decode keys using their desired scan code tables.
 *
 * @{
 */

/**
 * @brief PS/2 callback called when user types or click a mouse.
 *
 * @param dev   Pointer to the device structure for the driver instance.
 * @param data  Data byte passed pack to the user.
 */
typedef void (*ps2_callback_t)(const struct device *dev, uint8_t data);

/**
 * @def_driverbackendgroup{PS/2,ps2_interface}
 * @ingroup ps2_interface
 * @{
 */

/**
 * @brief Callback API for configuring a PS/2 device
 * @see ps2_config() for argument descriptions.
 */
typedef int (*ps2_config_t)(const struct device *dev,
			    ps2_callback_t callback_isr);

/**
 * @brief Callback API for reading from a PS/2 device
 * @see ps2_read() for argument descriptions.
 */
typedef int (*ps2_read_t)(const struct device *dev, uint8_t *value);

/**
 * @brief Callback API for writing to a PS/2 device
 * @see ps2_write() for argument descriptions.
 */
typedef int (*ps2_write_t)(const struct device *dev, uint8_t value);

/**
 * @brief Callback API for disabling a PS/2 device callback
 */
typedef int (*ps2_disable_callback_t)(const struct device *dev);

/**
 * @brief Callback API for enabling a PS/2 device callback
 * @see ps2_enable_callback() for argument descriptions.
 */
typedef int (*ps2_enable_callback_t)(const struct device *dev);

/**
 * @driver_ops{PS/2}
 */
__subsystem struct ps2_driver_api {
	/** @driver_ops_mandatory @copybrief ps2_config() */
	ps2_config_t config;
	/** @driver_ops_mandatory @copybrief ps2_read() */
	ps2_read_t read;
	/** @driver_ops_mandatory @copybrief ps2_write() */
	ps2_write_t write;
	/** @driver_ops_optional @copybrief ps2_disable_callback() */
	ps2_disable_callback_t disable_callback;
	/** @driver_ops_optional @copybrief ps2_enable_callback() */
	ps2_enable_callback_t enable_callback;
};

/** @} */

/**
 * @brief Configure a PS/2 device instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback_isr called when PS/2 devices reply to a configuration
 * command or when a mouse/keyboard send data to the client application.
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int ps2_config(const struct device *dev,
			 ps2_callback_t callback_isr);

static inline int z_impl_ps2_config(const struct device *dev,
				    ps2_callback_t callback_isr)
{
	return DEVICE_API_GET(ps2, dev)->config(dev, callback_isr);
}

/**
 * @brief Write to a PS/2 device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Data for the PS2 device.
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int ps2_write(const struct device *dev, uint8_t value);

static inline int z_impl_ps2_write(const struct device *dev, uint8_t value)
{
	return DEVICE_API_GET(ps2, dev)->write(dev, value);
}

/**
 * @brief Read slave-to-host values from PS/2 device.
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Pointer used for reading the PS/2 device.
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int ps2_read(const struct device *dev,  uint8_t *value);

static inline int z_impl_ps2_read(const struct device *dev, uint8_t *value)
{
	return DEVICE_API_GET(ps2, dev)->read(dev, value);
}

/**
 * @brief Enables callback.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int ps2_enable_callback(const struct device *dev);

static inline int z_impl_ps2_enable_callback(const struct device *dev)
{
	const struct ps2_driver_api *api = DEVICE_API_GET(ps2, dev);

	if (api->enable_callback == NULL) {
		return -ENOSYS;
	}

	return api->enable_callback(dev);
}

/**
 * @brief Disables callback.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int ps2_disable_callback(const struct device *dev);

static inline int z_impl_ps2_disable_callback(const struct device *dev)
{
	const struct ps2_driver_api *api = DEVICE_API_GET(ps2, dev);

	if (api->disable_callback == NULL) {
		return -ENOSYS;
	}

	return api->disable_callback(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/ps2.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PS2_H_ */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for PS/2 devices such as keyboard and mouse.
 * Callers of this API are responsible for setting the typematic rate
 * and decode keys using their desired scan code tables.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PS2_H_
#define ZEPHYR_INCLUDE_DRIVERS_PS2_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PS/2 Driver APIs
 * @defgroup ps2_interface PS/2 Driver APIs
 * @ingroup io_interfaces
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
 * @cond INTERNAL_HIDDEN
 *
 * PS2 driver API definition and system call entry points
 *
 * (Internal use only.)
 */
typedef int (*ps2_config_t)(const struct device *dev,
			    ps2_callback_t callback_isr);
typedef int (*ps2_read_t)(const struct device *dev, uint8_t *value);
typedef int (*ps2_write_t)(const struct device *dev, uint8_t value);
typedef int (*ps2_disable_callback_t)(const struct device *dev);
typedef int (*ps2_enable_callback_t)(const struct device *dev);

__subsystem struct ps2_driver_api {
	ps2_config_t config;
	ps2_read_t read;
	ps2_write_t write;
	ps2_disable_callback_t disable_callback;
	ps2_enable_callback_t enable_callback;
};
/**
 * @endcond
 */

/**
 * @brief Configure a ps2 instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback_isr called when PS/2 devices reply to a configuration
 * command or when a mouse/keyboard send data to the client application.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int ps2_config(const struct device *dev,
			 ps2_callback_t callback_isr);

static inline int z_impl_ps2_config(const struct device *dev,
				    ps2_callback_t callback_isr)
{
	const struct ps2_driver_api *api =
				(struct ps2_driver_api *)dev->api;

	return api->config(dev, callback_isr);
}

/**
 * @brief Write to PS/2 device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Data for the PS2 device.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int ps2_write(const struct device *dev, uint8_t value);

static inline int z_impl_ps2_write(const struct device *dev, uint8_t value)
{
	const struct ps2_driver_api *api =
			(const struct ps2_driver_api *)dev->api;

	return api->write(dev, value);
}

/**
 * @brief Read slave-to-host values from PS/2 device.
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Pointer used for reading the PS/2 device.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int ps2_read(const struct device *dev,  uint8_t *value);

static inline int z_impl_ps2_read(const struct device *dev, uint8_t *value)
{
	const struct ps2_driver_api *api =
			(const struct ps2_driver_api *)dev->api;

	return api->read(dev, value);
}

/**
 * @brief Enables callback.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int ps2_enable_callback(const struct device *dev);

static inline int z_impl_ps2_enable_callback(const struct device *dev)
{
	const struct ps2_driver_api *api =
			(const struct ps2_driver_api *)dev->api;

	if (api->enable_callback == NULL) {
		return -ENOTSUP;
	}

	return api->enable_callback(dev);
}

/**
 * @brief Disables callback.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int ps2_disable_callback(const struct device *dev);

static inline int z_impl_ps2_disable_callback(const struct device *dev)
{
	const struct ps2_driver_api *api =
			(const struct ps2_driver_api *)dev->api;

	if (api->disable_callback == NULL) {
		return -ENOTSUP;
	}

	return api->disable_callback(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/ps2.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PS2_H_ */

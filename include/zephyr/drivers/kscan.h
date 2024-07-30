/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for Keyboard scan matrix devices.
 * The scope of this API is simply to report which key event was triggered
 * and users can later decode keys using their desired scan code tables in
 * their application. In addition, typematic rate and delay can easily be
 * implemented using a timer if desired.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_KB_SCAN_H_
#define ZEPHYR_INCLUDE_DRIVERS_KB_SCAN_H_

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief KSCAN APIs
 * @defgroup kscan_interface Keyboard Scan Driver APIs
 * @since 2.1
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Keyboard scan callback called when user press/release
 * a key on a matrix keyboard.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param row Describes row change.
 * @param column Describes column change.
 * @param pressed Describes the kind of key event.
 */
typedef void (*kscan_callback_t)(const struct device *dev, uint32_t row,
				 uint32_t column,
				 bool pressed);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Keyboard scan driver API definition and system call entry points.
 *
 * (Internal use only.)
 */
typedef int (*kscan_config_t)(const struct device *dev,
			      kscan_callback_t callback);
typedef int (*kscan_disable_callback_t)(const struct device *dev);
typedef int (*kscan_enable_callback_t)(const struct device *dev);

__subsystem struct kscan_driver_api {
	kscan_config_t config;
	kscan_disable_callback_t disable_callback;
	kscan_enable_callback_t enable_callback;
};
/**
 * @endcond
 */

/**
 * @brief Configure a Keyboard scan instance.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback called when keyboard devices reply to a keyboard
 * event such as key pressed/released.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int kscan_config(const struct device *dev,
			     kscan_callback_t callback);

static inline int z_impl_kscan_config(const struct device *dev,
					kscan_callback_t callback)
{
	const struct kscan_driver_api *api =
				(struct kscan_driver_api *)dev->api;

	return api->config(dev, callback);
}
/**
 * @brief Enables callback.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
__syscall int kscan_enable_callback(const struct device *dev);

static inline int z_impl_kscan_enable_callback(const struct device *dev)
{
	const struct kscan_driver_api *api =
			(const struct kscan_driver_api *)dev->api;

	if (api->enable_callback == NULL) {
		return -ENOSYS;
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
__syscall int kscan_disable_callback(const struct device *dev);

static inline int z_impl_kscan_disable_callback(const struct device *dev)
{
	const struct kscan_driver_api *api =
			(const struct kscan_driver_api *)dev->api;

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

#include <zephyr/syscalls/kscan.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_KB_SCAN_H_ */

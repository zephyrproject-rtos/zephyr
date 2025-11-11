/*
 * Copyright 2024 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup haptics_interface
 * @brief Main header file for haptics driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_H_

/**
 * @brief Interfaces for haptic devices.
 * @defgroup haptics_interface Haptics
 * @ingroup io_interfaces
 * @{
 *
 * @defgroup haptics_interface_ext Device-specific Haptics API extensions
 *
 * @{
 * @}
 */

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef haptics_stop_output_t
 * @brief Set the haptic device to stop output
 * @param dev Pointer to the device structure for haptic device instance
 */
typedef int (*haptics_stop_output_t)(const struct device *dev);

/**
 * @typedef haptics_start_output_t
 * @brief Set the haptic device to start output for a playback event
 */
typedef int (*haptics_start_output_t)(const struct device *dev);

/**
 * @brief Haptic device API
 */
__subsystem struct haptics_driver_api {
	haptics_start_output_t start_output;
	haptics_stop_output_t stop_output;
};

/**
 * @brief Set the haptic device to start output for a playback event
 *
 * @param dev Pointer to the device structure for haptic device instance
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
__syscall int haptics_start_output(const struct device *dev);

static inline int z_impl_haptics_start_output(const struct device *dev)
{
	const struct haptics_driver_api *api = (const struct haptics_driver_api *)dev->api;

	return api->start_output(dev);
}

/**
 * @brief Set the haptic device to stop output for a playback event
 *
 * @param dev Pointer to the device structure for haptic device instance
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
__syscall int haptics_stop_output(const struct device *dev);

static inline int z_impl_haptics_stop_output(const struct device *dev)
{
	const struct haptics_driver_api *api = (const struct haptics_driver_api *)dev->api;

	return api->stop_output(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <syscalls/haptics.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HAPTICS_H_ */

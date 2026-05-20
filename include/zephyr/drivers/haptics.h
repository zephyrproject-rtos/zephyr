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
 * @brief Haptics error types
 */
enum haptics_error_type {
	HAPTICS_ERROR_OVERCURRENT = BIT(0),     /**< Output overcurrent error */
	HAPTICS_ERROR_OVERTEMPERATURE = BIT(1), /**< Device overtemperature error */
	HAPTICS_ERROR_UNDERVOLTAGE = BIT(2),    /**< Power source undervoltage error */
	HAPTICS_ERROR_OVERVOLTAGE = BIT(3),     /**< Power source overvoltage error */
	HAPTICS_ERROR_DC = BIT(4),              /**< Output direct-current error */

	/* Device-specific error codes can follow, refer to the device’s header file */
	/** @cond INTERNAL_HIDDEN */
	HAPTICS_ERROR_PRIV_START = BIT(5),
	/** @endcond */
};

/**
 * @brief Function type of callback invoked when a haptic device error occurs.
 *
 * @param dev Pointer to the haptic device
 * @param errors Device errors (bitmask of @ref haptics_error_type values)
 * @param user_data User data provided when the error callback was registered
 */
typedef void (*haptics_error_callback_t)(const struct device *dev, const uint32_t errors,
					 void *const user_data);

/**
 * @def_driverbackendgroup{Haptics,haptics_interface}
 * @{
 */

/**
 * @brief Set the haptic device to stop output.
 * See haptics_stop_output() for argument description.
 */
typedef int (*haptics_stop_output_t)(const struct device *dev);

/**
 * @brief Set the haptic device to start output for a playback event.
 * See haptics_start_output() for argument description.
 */
typedef int (*haptics_start_output_t)(const struct device *dev);

/**
 * @brief Register a callback function for haptics errors.
 * See haptics_register_error_callback() for argument description.
 */
typedef int (*haptics_register_error_callback_t)(const struct device *dev,
						 haptics_error_callback_t cb,
						 void *const user_data);

/**
 * @driver_ops{Haptics}
 */
__subsystem struct haptics_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief haptics_start_output
	 */
	haptics_start_output_t start_output;
	/**
	 * @driver_ops_mandatory @copybrief haptics_stop_output
	 */
	haptics_stop_output_t stop_output;
	/**
	 * @driver_ops_optional @copybrief haptics_register_error_callback
	 */
	haptics_register_error_callback_t register_error_callback;
};

/**
 * @}
 */

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
	return DEVICE_API_GET(haptics, dev)->start_output(dev);
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
	return DEVICE_API_GET(haptics, dev)->stop_output(dev);
}

/**
 * @brief Register a callback function for haptics errors
 *
 * @param dev Pointer to the haptic device
 * @param cb Callback function (of type @ref haptics_error_callback_t)
 * @param user_data User data to be provided back to the application via the callback
 *
 * @retval 0 if successful
 * @retval <0 if failed
 */
static inline int haptics_register_error_callback(const struct device *dev,
						  haptics_error_callback_t cb,
						  void *const user_data)
{
	const struct haptics_driver_api *api = DEVICE_API_GET(haptics, dev);

	if (api->register_error_callback == NULL) {
		return -ENOSYS;
	}

	return api->register_error_callback(dev, cb, user_data);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <syscalls/haptics.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HAPTICS_H_ */

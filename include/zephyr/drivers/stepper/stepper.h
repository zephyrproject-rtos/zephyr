/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup stepper_interface
 * @brief Main header file for stepper hardware driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for stepper motion controllers and hardware drivers.
 * @defgroup stepper_interface Stepper
 * @ingroup io_interfaces
 *
 * @defgroup stepper_hw_driver_interface Stepper Hardware Driver Interface
 * @brief Interfaces for stepper hardware drivers
 * @ingroup stepper_interface
 * @since 4.0
 * @version 0.9.0
 * @{
 */

/**
 * @brief Stepper Motor micro-step resolution options
 */
enum stepper_micro_step_resolution {
	/** Full step resolution */
	STEPPER_MICRO_STEP_1 = 1,
	/** 2 micro-steps per full step */
	STEPPER_MICRO_STEP_2 = 2,
	/** 4 micro-steps per full step */
	STEPPER_MICRO_STEP_4 = 4,
	/** 8 micro-steps per full step */
	STEPPER_MICRO_STEP_8 = 8,
	/** 16 micro-steps per full step */
	STEPPER_MICRO_STEP_16 = 16,
	/** 32 micro-steps per full step */
	STEPPER_MICRO_STEP_32 = 32,
	/** 64 micro-steps per full step */
	STEPPER_MICRO_STEP_64 = 64,
	/** 128 micro-steps per full step */
	STEPPER_MICRO_STEP_128 = 128,
	/** 256 micro-steps per full step */
	STEPPER_MICRO_STEP_256 = 256,
};

/**
 * @brief Macro to calculate the index of the microstep resolution
 * @param res Microstep resolution
 */
#define MICRO_STEP_RES_INDEX(res) LOG2(res)

#define VALID_MICRO_STEP_RES(res)                                                                  \
	((res) == STEPPER_MICRO_STEP_1 || (res) == STEPPER_MICRO_STEP_2 ||                 \
	 (res) == STEPPER_MICRO_STEP_4 || (res) == STEPPER_MICRO_STEP_8 ||                 \
	 (res) == STEPPER_MICRO_STEP_16 || (res) == STEPPER_MICRO_STEP_32 ||               \
	 (res) == STEPPER_MICRO_STEP_64 || (res) == STEPPER_MICRO_STEP_128 ||              \
	 (res) == STEPPER_MICRO_STEP_256)

/**
 * @brief Stepper Hardware Driver Events
 */
enum stepper_event {
	/** Stepper driver stall detected */
	STEPPER_EVENT_STALL_DETECTED = 0,
	/** Stepper driver fault detected */
	STEPPER_EVENT_FAULT_DETECTED = 1,
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Stepper hardware driver API definition and system call entry points.
 *
 */

/**
 * @brief Enable the stepper hardware driver
 *
 * @see stepper_enable() for details.
 */
typedef int (*stepper_enable_t)(const struct device *dev);

/**
 * @brief Disable the stepper hardware driver
 *
 * @see stepper_disable() for details.
 */
typedef int (*stepper_disable_t)(const struct device *dev);

/**
 * @brief Set the stepper micro-step resolution
 *
 * @see stepper_set_micro_step_res() for details.
 */
typedef int (*stepper_set_micro_step_res_t)(
	const struct device *dev, const enum stepper_micro_step_resolution resolution);

/**
 * @brief Get the stepper micro-step resolution
 *
 * @see stepper_get_micro_step_res() for details.
 */
typedef int (*stepper_get_micro_step_res_t)(const struct device *dev,
						enum stepper_micro_step_resolution *resolution);

/**
 * @brief Callback function for stepper driver events
 */
typedef void (*stepper_event_cb_t)(const struct device *dev, const enum stepper_event event,
				       void *user_data);

/**
 * @brief Set the callback function to be called when a stepper_event occurs
 *
 * @see stepper_set_event_cb() for details.
 */
typedef int (*stepper_set_event_cb_t)(const struct device *dev,
						stepper_event_cb_t callback, void *user_data);

/**
 * @driver_ops{Stepper Hardware Driver}
 */
__subsystem struct stepper_driver_api {
	stepper_enable_t enable;
	stepper_disable_t disable;
	stepper_set_micro_step_res_t set_micro_step_res;
	stepper_get_micro_step_res_t get_micro_step_res;
	stepper_set_event_cb_t set_event_cb;
};

/**
 * @endcond
 */

/**
 * @brief Enable stepper hardware driver
 *
 * @details Enabling the driver shall switch on the power stage and energize the coils.
 *
 * @param dev pointer to the device structure for the driver instance.
 *
 * @retval -EIO Error during Enabling
 * @retval 0 Success
 */
__syscall int stepper_enable(const struct device *dev);

static inline int z_impl_stepper_enable(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->enable(dev);
}

/**
 * @brief Disable stepper hardware driver
 *
 * @details Disabling the hw driver shall switch off the power stage and de-energize the coils.
 *
 * @param dev pointer to the device structure for the driver instance.
 *
 * @retval -ENOTSUP Disabling of hw driver is not supported.
 * @retval -EIO Error during Disabling
 * @retval 0 Success
 */
__syscall int stepper_disable(const struct device *dev);

static inline int z_impl_stepper_disable(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Set the micro-step resolution in stepper hardware driver
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param res micro-step resolution
 *
 * @retval -EIO General input / output error
 * @retval -EINVAL If the requested resolution is invalid
 * @retval -ENOTSUP If the requested resolution is not supported
 * @retval 0 Success
 */
__syscall int stepper_set_micro_step_res(const struct device *dev,
					     enum stepper_micro_step_resolution res);

static inline int z_impl_stepper_set_micro_step_res(const struct device *dev,
							enum stepper_micro_step_resolution res)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (!VALID_MICRO_STEP_RES(res)) {
		return -EINVAL;
	}
	return api->set_micro_step_res(dev, res);
}

/**
 * @brief Get the micro-step resolution in stepper hardware driver
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param res micro-step resolution
 *
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
__syscall int stepper_get_micro_step_res(const struct device *dev,
					     enum stepper_micro_step_resolution *res);

static inline int z_impl_stepper_get_micro_step_res(const struct device *dev,
							enum stepper_micro_step_resolution *res)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(res != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->get_micro_step_res(dev, res);
}

/**
 * @brief Set the callback function to be called when a stepper_event occurs
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param callback Callback function to be called when a stepper_event occurs
 * passing NULL will disable the callback
 * @param user_data User data to be passed to the callback function
 *
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_set_event_cb(const struct device *dev, stepper_event_cb_t callback,
				       void *user_data);

static inline int z_impl_stepper_set_event_cb(const struct device *dev,
						  stepper_event_cb_t cb, void *user_data)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_event_cb == NULL) {
		return -ENOSYS;
	}

	return api->set_event_cb(dev, cb, user_data);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/stepper.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_ */

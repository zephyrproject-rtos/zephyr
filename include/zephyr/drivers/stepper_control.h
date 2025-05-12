/*
 * SPDX-FileCopyright-Text: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-File-Identifier: Apache-2.0
 */

/**
 * @file drivers/stepper-control.h
 * @brief Public API for Stepper Control Driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_CONTROL_H_

/**
 * @brief Stepper Control Driver Interface
 * @defgroup stepper_controller_interface Stepper Control Driver Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

enum stepper_control_mode {
	/** Constant speed mode */
	STEPPER_CONTROL_MODE_CONSTANT_SPEED = 0,
	/** Ramp mode */
	STEPPER_CONTROL_MODE_RAMP = 1,
};

/**
 * @brief Stepper Events
 */
enum stepper_control_event {
	/** Steps set using move_by or move_to have been executed */
	STEPPER_CONTROL_EVENT_STEPS_COMPLETED = 0,
	/** Stall detected */
	STEPPER_CONTROL_EVENT_STALL_DETECTED = 1,
	/** Left end switch status changes to pressed */
	STEPPER_CONTROL_EVENT_LEFT_END_STOP_DETECTED = 2,
	/** Right end switch status changes to pressed */
	STEPPER_CONTROL_EVENT_RIGHT_END_STOP_DETECTED = 3,
	/** Stepper has stopped */
	STEPPER_CONTROL_EVENT_STOPPED = 4,
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Stepper Control driver API definition and system call entry points.
 *
 */

/**
 * @brief Move the stepper to a specific position
 */
typedef int (*stepper_control_move_to_t)(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Move the stepper by a specific number of steps
 */
typedef int (*stepper_control_move_by_t)(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Run the stepper in a specific direction
 */
typedef int (*stepper_control_run_t)(const struct device *dev,
				     const enum stepper_direction direction);

/**
 * @brief Stop the stepper
 *
 * @see stepper_stop() for details.
 */
typedef int (*stepper_control_stop_t)(const struct device *dev);

/**
 * @brief Check if the stepper is currently moving
 */
typedef int (*stepper_is_moving_t)(const struct device *dev, bool *is_moving);

/**
 * @brief Set the step interval of the stepper motor
 */
typedef int (*stepper_control_set_step_interval_t)(const struct device *dev,
						   const uint64_t microstep_interval_ns);

/**
 * @brief Get the actual position of the stepper motor
 */
typedef int (*stepper_control_get_actual_position_t)(const struct device *dev, int32_t *value);

/**
 * @brief Set the reference position of the stepper motor
 */
typedef int (*stepper_control_set_reference_position_t)(const struct device *dev,
							const int32_t value);

/**
 * @brief Set the stepper control mode
 */
typedef int (*stepper_control_set_mode_t)(const struct device *dev,
					  const enum stepper_control_mode mode);

/**
 * @brief Callback function for stepper events
 */
typedef void (*stepper_control_event_callback_t)(const struct device *dev,
						 const enum stepper_control_event event,
						 void *user_data);

/**
 * @brief Set the callback function to be called when a stepper event occurs
 *
 * @see stepper_set_event_callback() for details.
 */
typedef int (*stepper_control_set_event_callback_t)(const struct device *dev,
						    stepper_control_event_callback_t callback,
						    void *user_data);

/**
 * @brief Stepper Control Driver API
 */
__subsystem struct stepper_control_driver_api {
	stepper_control_move_to_t move_to;
	stepper_control_move_by_t move_by;
	stepper_control_run_t run;
	stepper_control_stop_t stop;
	stepper_control_set_step_interval_t set_step_interval;
	stepper_control_get_actual_position_t get_actual_position;
	stepper_control_set_reference_position_t set_reference_position;
	stepper_control_set_mode_t set_mode;
	stepper_is_moving_t is_moving;
	stepper_control_set_event_callback_t set_event_callback;
};

/**
 * @endcond
 */

/**
 * @brief Set the absolute target position of the stepper
 *
 * @details The stepper will move to the given micro-steps position from the reference position.
 * This function is non-blocking.
 *
 * @param dev pointer to the stepper driver instance
 * @param micro_steps target position to set in micro-steps
 *
 * @retval -ECANCELED If the stepper is disabled
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_control_move_to(const struct device *dev, const int32_t micro_steps);

static inline int z_impl_stepper_control_move_to(const struct device *dev,
						 const int32_t micro_steps)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->move_to(dev, micro_steps);
}

/**
 * @brief Set the micro-steps to be moved from the current position i.e. relative movement
 *
 * @details The stepper will move by the given number of micro-steps from the current position.
 * This function is non-blocking.
 *
 * @param dev pointer to the stepper driver instance
 * @param micro_steps target micro-steps to be moved from the current position
 *
 * @retval -ECANCELED If the stepper is disabled
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
__syscall int stepper_control_move_by(const struct device *dev, const int32_t micro_steps);

static inline int z_impl_stepper_control_move_by(const struct device *dev,
						 const int32_t micro_steps)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->move_by(dev, micro_steps);
}

/**
 * @brief Run the stepper with a given step interval in a given direction
 *
 * @details The stepper shall be set into motion and run continuously until
 * stalled or stopped using some other command, for instance, stepper_enable(false). This
 * function is non-blocking.
 *
 * @param dev pointer to the stepper driver instance
 * @param direction The direction to set
 *
 * @retval -ECANCELED If the stepper is disabled
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_control_run(const struct device *dev, const enum stepper_direction direction);

static inline int z_impl_stepper_control_run(const struct device *dev,
					     const enum stepper_direction direction)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->run(dev, direction);
}

/**
 * @brief Stop the stepper
 *
 * @details The stepper shall be stopped immediately. This function is non-blocking.
 *
 * @param dev pointer to the stepper driver instance
 *
 * @retval -ECANCELED If the stepper is disabled
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_control_stop(const struct device *dev);

static inline int z_impl_stepper_control_stop(const struct device *dev)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->stop(dev);
}

/**
 * @brief Set the time interval between steps in nanoseconds with immediate effect.
 *
 * @note Setting step interval does not set the stepper into motion, a combination of
 * set_microstep_interval and move is required to set the stepper into motion.
 *
 * @param dev pointer to the stepper driver instance
 * @param microstep_interval_ns time interval between steps in nanoseconds
 *
 * @retval -EIO General input / output error
 * @retval -EINVAL If the requested step interval is not supported
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_control_set_step_interval(const struct device *dev,
						const uint64_t microstep_interval_ns);
static inline int z_impl_stepper_control_set_step_interval(const struct device *dev,
							   const uint64_t microstep_interval_ns)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->set_step_interval(dev, microstep_interval_ns);
}

/**
 * @brief Get the actual a.k.a reference position of the stepper
 *
 * @param dev pointer to the stepper driver instance
 * @param value The actual position to get in micro-steps
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_control_get_actual_position(const struct device *dev, int32_t *value);
static inline int z_impl_stepper_control_get_actual_position(const struct device *dev,
							     int32_t *value)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->get_actual_position(dev, value);
}

/**
 * @brief Set the reference position of the stepper
 *
 * @param dev Pointer to the stepper driver instance.
 * @param value The reference position to set in micro-steps.
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_control_set_reference_position(const struct device *dev, const int32_t value);

static inline int z_impl_stepper_control_set_reference_position(const struct device *dev,
								const int32_t value)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->set_reference_position(dev, value);
}

__syscall int stepper_control_set_mode(const struct device *dev,
				       const enum stepper_control_mode mode);
static inline int z_impl_stepper_control_set_mode(const struct device *dev,
						  const enum stepper_control_mode mode)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->set_mode(dev, mode);
}

/**
 * @brief Check if the stepper is currently moving
 *
 * @param dev pointer to the stepper driver instance
 * @param is_moving Pointer to a boolean to store the moving status of the stepper
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_control_is_moving(const struct device *dev, bool *is_moving);

static inline int z_impl_stepper_control_is_moving(const struct device *dev, bool *is_moving)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->is_moving(dev, is_moving);
}

/**
 * @brief Set the callback function to be called when a stepper event occurs
 *
 * @param dev pointer to the stepper driver instance
 * @param callback Callback function to be called when a stepper event occurs
 * passing NULL will disable the callback
 * @param user_data User data to be passed to the callback function
 *
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_control_set_event_callback(const struct device *dev,
						 stepper_control_event_callback_t callback,
						 void *user_data);

static inline int z_impl_stepper_control_set_event_callback(
	const struct device *dev, stepper_control_event_callback_t callback, void *user_data)
{
	const struct stepper_control_driver_api *api = dev->api;

	return api->set_event_callback(dev, callback, user_data);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/stepper_control.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_CONTROL_H_ */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup stepper_ctrl_interface
 * @brief Main header file for stepper motion controller driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_CTRL_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_CTRL_H_

/**
 * @brief Interfaces for stepper motion controllers.
 * @defgroup stepper_ctrl_interface Stepper Motion Controller Interface
 * @since 4.0
 * @version 0.9.0
 * @ingroup stepper_interface
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stepper Motion Controller direction options
 */
enum stepper_ctrl_direction {
	/** Negative direction */
	STEPPER_CTRL_DIRECTION_NEGATIVE = 0,
	/** Positive direction */
	STEPPER_CTRL_DIRECTION_POSITIVE = 1,
};

/**
 * @brief Stepper Motion Controller run mode options
 */
enum stepper_ctrl_run_mode {
	/** Hold Mode */
	STEPPER_CTRL_RUN_MODE_HOLD = 0,
	/** Position Mode */
	STEPPER_CTRL_RUN_MODE_POSITION = 1,
	/** Velocity Mode */
	STEPPER_CTRL_RUN_MODE_VELOCITY = 2,
};

/**
 * @brief Stepper Motion Controller Events
 */
enum stepper_ctrl_event {
	/** Steps set using move_by or move_to have been executed */
	STEPPER_CTRL_EVENT_STEPS_COMPLETED = 0,
	/** Left end switch status changes to pressed */
	STEPPER_CTRL_EVENT_LEFT_END_STOP_DETECTED = 1,
	/** Right end switch status changes to pressed */
	STEPPER_CTRL_EVENT_RIGHT_END_STOP_DETECTED = 2,
	/** Stepper has stopped */
	STEPPER_CTRL_EVENT_STOPPED = 3,
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Stepper Motion Controller driver API definition and entry points.
 *
 */

/**
 * @brief Set the reference position of the stepper motion controller
 *
 * @see stepper_ctrl_set_reference_position() for details.
 */
typedef int (*stepper_ctrl_set_reference_position_t)(const struct device *dev, const int32_t value);

/**
 * @brief Get the actual a.k.a reference position of the stepper motion controller
 *
 * @see stepper_ctrl_get_actual_position() for details.
 */
typedef int (*stepper_ctrl_get_actual_position_t)(const struct device *dev, int32_t *value);

/**
 * @brief Callback function for stepper motion controller events
 */
typedef void (*stepper_ctrl_event_callback_t)(const struct device *dev,
					      const enum stepper_ctrl_event event, void *user_data);

/**
 * @brief Set the callback function to be called when a stepper motion controller event occurs
 *
 * @see stepper_ctrl_set_event_cb() for details.
 */
typedef int (*stepper_ctrl_set_event_cb_t)(const struct device *dev,
					   stepper_ctrl_event_callback_t callback, void *user_data);

/**
 * @brief Set the time interval between steps in nanoseconds.
 *
 * @see stepper_ctrl_set_microstep_interval() for details.
 */
typedef int (*stepper_ctrl_set_microstep_interval_t)(const struct device *dev,
						     const uint64_t microstep_interval_ns);
/**
 * @brief Move the stepper relatively by a given number of micro-steps.
 *
 * @see stepper_ctrl_move_by() for details.
 */
typedef int (*stepper_ctrl_move_by_t)(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Move the stepper to an absolute position in micro-steps.
 *
 * @see stepper_ctrl_move_to() for details.
 */
typedef int (*stepper_ctrl_move_to_t)(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Run the stepper with a given step interval in a given direction
 *
 * @see stepper_ctrl_run() for details.
 */
typedef int (*stepper_ctrl_run_t)(const struct device *dev,
				  const enum stepper_ctrl_direction direction);

/**
 * @brief Stop the stepper
 *
 * @see stepper_ctrl_stop() for details.
 */
typedef int (*stepper_ctrl_stop_t)(const struct device *dev);

/**
 * @brief Is the target position fo the stepper reached
 *
 * @see stepper_ctrl_is_moving() for details.
 */
typedef int (*stepper_ctrl_is_moving_t)(const struct device *dev, bool *is_moving);

/**
 * @driver_ops{Stepper Motion Controller Driver API}
 */
__subsystem struct stepper_ctrl_driver_api {
	stepper_ctrl_set_reference_position_t set_reference_position;
	stepper_ctrl_get_actual_position_t get_actual_position;
	stepper_ctrl_set_event_cb_t set_event_cb;
	stepper_ctrl_set_microstep_interval_t set_microstep_interval;
	stepper_ctrl_move_by_t move_by;
	stepper_ctrl_move_to_t move_to;
	stepper_ctrl_run_t run;
	stepper_ctrl_stop_t stop;
	stepper_ctrl_is_moving_t is_moving;
};

/**
 * @endcond
 */

/**
 * @brief Set the reference position
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param value The reference position to set in micro-steps.
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_ctrl_set_reference_position(const struct device *dev, const int32_t value);

static inline int z_impl_stepper_ctrl_set_reference_position(const struct device *dev,
							     const int32_t value)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	if (api->set_reference_position == NULL) {
		return -ENOSYS;
	}
	return api->set_reference_position(dev, value);
}

/**
 * @brief Get the actual step count.
 * @note This function does not guarantee that the returned position is the exact current
 * position. For precise positioning, encoders should be used in addition to the stepper motion
 * controller.
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param value The actual position to get in micro-steps
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */

__syscall int stepper_ctrl_get_actual_position(const struct device *dev, int32_t *value);

static inline int z_impl_stepper_ctrl_get_actual_position(const struct device *dev, int32_t *value)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(value != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	if (api->get_actual_position == NULL) {
		return -ENOSYS;
	}
	return api->get_actual_position(dev, value);
}

/**
 * @brief Set the callback function to be called when a stepper motion controller event occurs
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param callback Callback function to be called when a stepper motion controller event occurs
 * passing NULL will disable the callback
 * @param user_data User data to be passed to the callback function
 *
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */

__syscall int stepper_ctrl_set_event_cb(const struct device *dev,
					stepper_ctrl_event_callback_t callback, void *user_data);

static inline int z_impl_stepper_ctrl_set_event_cb(const struct device *dev,
						   stepper_ctrl_event_callback_t callback,
						   void *user_data)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	if (api->set_event_cb == NULL) {
		return -ENOSYS;
	}
	return api->set_event_cb(dev, callback, user_data);
}

/**
 * @brief Set the time interval between steps in nanoseconds with immediate effect.
 *
 * @note Setting step interval does not set the stepper into motion, a combination of
 * set_microstep_interval and move is required to set the stepper into motion.
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param microstep_interval_ns time interval between steps in nanoseconds
 *
 * @retval -EIO General input / output error
 * @retval -EINVAL If the requested step interval is not supported
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_ctrl_set_microstep_interval(const struct device *dev,
						  const uint64_t microstep_interval_ns);

static inline int z_impl_stepper_ctrl_set_microstep_interval(const struct device *dev,
							     const uint64_t microstep_interval_ns)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	if (api->set_microstep_interval == NULL) {
		return -ENOSYS;
	}
	return api->set_microstep_interval(dev, microstep_interval_ns);
}

/**
 * @brief Set the micro-steps to be moved from the current position i.e. relative movement
 *
 * @note The stepper will move by the given number of micro-steps from the current position.
 * This function is non-blocking.
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param micro_steps target micro-steps to be moved from the current position
 *
 * @retval -EIO General input / output error
 * @retval -EINVAL If the timing for steps is incorrectly configured
 * @retval 0 Success
 */
__syscall int stepper_ctrl_move_by(const struct device *dev, const int32_t micro_steps);

static inline int z_impl_stepper_ctrl_move_by(const struct device *dev, const int32_t micro_steps)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	return api->move_by(dev, micro_steps);
}

/**
 * @brief Set the absolute target position of the stepper
 *
 * @note The stepper will move to the given micro-steps position from the reference position.
 * This function is non-blocking.
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param micro_steps target position to set in micro-steps
 *
 * @retval -EIO General input / output error
 * @retval -EINVAL If the timing for steps is incorrectly configured
 * @retval 0 Success
 */
__syscall int stepper_ctrl_move_to(const struct device *dev, const int32_t micro_steps);

static inline int z_impl_stepper_ctrl_move_to(const struct device *dev, const int32_t micro_steps)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	return api->move_to(dev, micro_steps);
}

/**
 * @brief Run the stepper with a given step interval in a given direction
 *
 * @note The stepper shall be set into motion and run continuously until
 * stalled or stopped using some other command, for instance, stepper_stop(). This
 * function is non-blocking.
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param direction The direction to set
 *
 * @retval -EIO General input / output error
 * @retval -EINVAL If the timing for steps is incorrectly configured
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_ctrl_run(const struct device *dev,
			       const enum stepper_ctrl_direction direction);

static inline int z_impl_stepper_ctrl_run(const struct device *dev,
					  const enum stepper_ctrl_direction direction)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	if (api->run == NULL) {
		return -ENOSYS;
	}
	return api->run(dev, direction);
}

/**
 * @brief Stop the stepper
 * @note Cancel all active movements.
 *
 * @param dev pointer to the device structure for the driver instance.
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_ctrl_stop(const struct device *dev);

static inline int z_impl_stepper_ctrl_stop(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	if (api->stop == NULL) {
		return -ENOSYS;
	}
	return api->stop(dev);
}

/**
 * @brief Check if the stepper is currently moving
 *
 * @param dev pointer to the device structure for the driver instance.
 * @param is_moving Pointer to a boolean to store the moving status of the stepper
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_ctrl_is_moving(const struct device *dev, bool *is_moving);

static inline int z_impl_stepper_ctrl_is_moving(const struct device *dev, bool *is_moving)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(is_moving != NULL);
	const struct stepper_ctrl_driver_api *api = dev->api;

	if (api->is_moving == NULL) {
		return -ENOSYS;
	}
	return api->is_moving(dev, is_moving);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/stepper_ctrl.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_CTRL_H_ */

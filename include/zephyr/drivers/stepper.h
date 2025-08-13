/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/stepper.h
 * @brief Public API for Stepper Driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_

/**
 * @brief Stepper Driver Interface
 * @defgroup stepper_interface Stepper Driver Interface
 * @since 4.0
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if the stepper index is valid
 * @param stepper_idx Stepper index to check
 * @param max_num_stepper Maximum number of steppers supported by the device
 * @param dev Pointer to the device structure
 */
#define CHECK_STEPPER_IDX(dev, stepper_idx, max_num_stepper)                                       \
	__ASSERT(stepper_idx < max_num_stepper,                                                    \
		 "Invalid stepper index %d, max is %d for device %s", stepper_idx,                 \
		 max_num_stepper, dev->name)

/**
 * @brief Stepper Motor direction options
 */
enum stepper_direction {
	/** Negative direction */
	STEPPER_DIRECTION_NEGATIVE = 0,
	/** Positive direction */
	STEPPER_DIRECTION_POSITIVE = 1,
};

/**
 * @brief Stepper Motor run mode options
 */
enum stepper_run_mode {
	/** Hold Mode */
	STEPPER_RUN_MODE_HOLD = 0,
	/** Position Mode*/
	STEPPER_RUN_MODE_POSITION = 1,
	/** Velocity Mode */
	STEPPER_RUN_MODE_VELOCITY = 2,
};

/**
 * @brief Stepper Events
 */
enum stepper_event {
	/** Steps set using move_by or move_to have been executed */
	STEPPER_EVENT_STEPS_COMPLETED = 0,
	/** Left end switch status changes to pressed */
	STEPPER_EVENT_LEFT_END_STOP_DETECTED = 1,
	/** Right end switch status changes to pressed */
	STEPPER_EVENT_RIGHT_END_STOP_DETECTED = 2,
	/** Stepper has stopped */
	STEPPER_EVENT_STOPPED = 3,
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Stepper driver API definition and system call entry points.
 *
 */

/**
 * @brief Set the reference position of the stepper
 *
 * @see stepper_set_actual_position() for details.
 */
typedef int (*stepper_set_reference_position_t)(const struct device *dev,
						const uint8_t stepper_index, const int32_t value);

/**
 * @brief Get the actual a.k.a reference position of the stepper
 *
 * @see stepper_get_actual_position() for details.
 */
typedef int (*stepper_get_actual_position_t)(const struct device *dev, const uint8_t stepper_index,
					     int32_t *value);

/**
 * @brief Callback function for stepper events
 */
typedef void (*stepper_event_callback_t)(const struct device *dev, const uint8_t stepper_index,
					 const enum stepper_event event, void *user_data);

/**
 * @brief Set the callback function to be called when a stepper event occurs
 *
 * @see stepper_set_event_callback() for details.
 */
typedef int (*stepper_set_event_callback_t)(const struct device *dev, const uint8_t stepper_index,
					    stepper_event_callback_t callback, void *user_data);

/**
 * @brief Set the time interval between steps in nanoseconds.
 *
 * @see stepper_set_microstep_interval() for details.
 */
typedef int (*stepper_set_microstep_interval_t)(const struct device *dev,
						const uint8_t stepper_index,
						const uint64_t microstep_interval_ns);
/**
 * @brief Move the stepper relatively by a given number of micro-steps.
 *
 * @see stepper_move_by() for details.
 */
typedef int (*stepper_move_by_t)(const struct device *dev, const uint8_t stepper_index,
				 const int32_t micro_steps);

/**
 * @brief Move the stepper to an absolute position in micro-steps.
 *
 * @see stepper_move_to() for details.
 */
typedef int (*stepper_move_to_t)(const struct device *dev, const uint8_t stepper_index,
				 const int32_t micro_steps);

/**
 * @brief Run the stepper with a given step interval in a given direction
 *
 * @see stepper_run() for details.
 */
typedef int (*stepper_run_t)(const struct device *dev, const uint8_t stepper_index,
			     const enum stepper_direction direction);

/**
 * @brief Stop the stepper
 *
 * @see stepper_stop() for details.
 */
typedef int (*stepper_stop_t)(const struct device *dev, const uint8_t stepper_index);

/**
 * @brief Is the target position fo the stepper reached
 *
 * @see stepper_is_moving() for details.
 */
typedef int (*stepper_is_moving_t)(const struct device *dev, const uint8_t stepper_index,
				   bool *is_moving);

/**
 * @brief Stepper Driver API
 */
__subsystem struct stepper_driver_api {
	stepper_set_reference_position_t set_reference_position;
	stepper_get_actual_position_t get_actual_position;
	stepper_set_event_callback_t set_event_callback;
	stepper_set_microstep_interval_t set_microstep_interval;
	stepper_move_by_t move_by;
	stepper_move_to_t move_to;
	stepper_run_t run;
	stepper_stop_t stop;
	stepper_is_moving_t is_moving;
};

/**
 * @endcond
 */

/**
 * @brief Set the reference position of the stepper
 *
 * @param dev Pointer to the stepper controller instance.
 * @param stepper_index The index of the stepper motor to set the reference position for.
 * @param value The reference position to set in micro-steps.
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_set_reference_position(const struct device *dev, const uint8_t stepper_index,
					     int32_t value);

static inline int z_impl_stepper_set_reference_position(const struct device *dev,
							const uint8_t stepper_index,
							const int32_t value)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_reference_position == NULL) {
		return -ENOSYS;
	}
	return api->set_reference_position(dev, stepper_index, value);
}

/**
 * @brief Get the actual a.k.a reference position of the stepper
 *
 * @param dev pointer to the stepper controller instance
 * @param stepper_index The index of the stepper motor to get the actual position for.
 * @param value The actual position to get in micro-steps
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_get_actual_position(const struct device *dev, const uint8_t stepper_index,
					  int32_t *value);

static inline int z_impl_stepper_get_actual_position(const struct device *dev,
						     const uint8_t stepper_index, int32_t *value)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(value != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->get_actual_position == NULL) {
		return -ENOSYS;
	}
	return api->get_actual_position(dev, stepper_index, value);
}

/**
 * @brief Set the callback function to be called when a stepper event occurs
 *
 * @param dev pointer to the stepper controller instance
 * @param stepper_index The index of the stepper motor to set the event callback for.
 * @param callback Callback function to be called when a stepper event occurs
 * passing NULL will disable the callback
 * @param user_data User data to be passed to the callback function
 *
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_set_event_callback(const struct device *dev, const uint8_t stepper_index,
					 stepper_event_callback_t callback, void *user_data);

static inline int z_impl_stepper_set_event_callback(const struct device *dev,
						    const uint8_t stepper_index,
						    stepper_event_callback_t callback,
						    void *user_data)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_event_callback == NULL) {
		return -ENOSYS;
	}
	return api->set_event_callback(dev, stepper_index, callback, user_data);
}

/**
 * @brief Set the time interval between steps in nanoseconds with immediate effect.
 *
 * @note Setting step interval does not set the stepper into motion, a combination of
 * set_microstep_interval and move is required to set the stepper into motion.
 *
 * @param dev pointer to the stepper controller instance
 * @param stepper_index The index of the stepper motor to set the microstep interval for.
 * @param microstep_interval_ns time interval between steps in nanoseconds
 *
 * @retval -EIO General input / output error
 * @retval -EINVAL If the requested step interval is not supported
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_set_microstep_interval(const struct device *dev, const uint8_t stepper_index,
					     uint64_t microstep_interval_ns);

static inline int z_impl_stepper_set_microstep_interval(const struct device *dev,
							const uint8_t stepper_index,
							const uint64_t microstep_interval_ns)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_microstep_interval == NULL) {
		return -ENOSYS;
	}
	return api->set_microstep_interval(dev, stepper_index, microstep_interval_ns);
}

/**
 * @brief Set the micro-steps to be moved from the current position i.e. relative movement
 *
 * @details The stepper will move by the given number of micro-steps from the current position.
 * This function is non-blocking.
 *
 * @param dev pointer to the stepper controller instance
 * @param stepper_index The index of the stepper motor to set the micro-steps for.
 * @param micro_steps target micro-steps to be moved from the current position
 *
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
__syscall int stepper_move_by(const struct device *dev, const uint8_t stepper_index,
			      int32_t micro_steps);

static inline int z_impl_stepper_move_by(const struct device *dev, const uint8_t stepper_index,
					 const int32_t micro_steps)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->move_by(dev, stepper_index, micro_steps);
}

/**
 * @brief Set the absolute target position of the stepper
 *
 * @details The stepper will move to the given micro-steps position from the reference position.
 * This function is non-blocking.
 *
 * @param dev pointer to the stepper controller instance
 * @param stepper_index The index of the stepper motor to set the target position for.
 * @param micro_steps target position to set in micro-steps
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_move_to(const struct device *dev, const uint8_t stepper_index,
			      int32_t micro_steps);

static inline int z_impl_stepper_move_to(const struct device *dev, const uint8_t stepper_index,
					 const int32_t micro_steps)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->move_to == NULL) {
		return -ENOSYS;
	}
	return api->move_to(dev, stepper_index, micro_steps);
}

/**
 * @brief Run the stepper with a given step interval in a given direction
 *
 * @details The stepper shall be set into motion and run continuously until
 * stalled or stopped using some other command, for instance, stepper_stop(). This
 * function is non-blocking.
 *
 * @param dev pointer to the stepper controller instance
 * @param stepper_index The index of the stepper motor to run.
 * @param direction The direction to set
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_run(const struct device *dev, const uint8_t stepper_index,
			  enum stepper_direction direction);

static inline int z_impl_stepper_run(const struct device *dev, const uint8_t stepper_index,
				     const enum stepper_direction direction)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->run == NULL) {
		return -ENOSYS;
	}
	return api->run(dev, stepper_index, direction);
}

/**
 * @brief Stop the stepper
 * @details Cancel all active movements.
 *
 * @param dev pointer to the stepper controller instance
 * @param stepper_index The index of the stepper motor to stop.
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_stop(const struct device *dev, const uint8_t stepper_index);

static inline int z_impl_stepper_stop(const struct device *dev, const uint8_t stepper_index)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->stop == NULL) {
		return -ENOSYS;
	}
	return api->stop(dev, stepper_index);
}

/**
 * @brief Check if the stepper is currently moving
 *
 * @param dev pointer to the stepper controller instance
 * @param stepper_index The index of the stepper motor to check if it is moving.
 * @param is_moving Pointer to a boolean to store the moving status of the stepper
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_is_moving(const struct device *dev, const uint8_t stepper_index,
				bool *is_moving);

static inline int z_impl_stepper_is_moving(const struct device *dev, const uint8_t stepper_index,
					   bool *is_moving)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(is_moving != NULL);
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->is_moving == NULL) {
		return -ENOSYS;
	}
	return api->is_moving(dev, stepper_index, is_moving);
}

/**
 * @}
 */

/**
 * @brief Stepper-Drv Driver Interface
 * @defgroup stepper_drv_interface Stepper Drv Driver Interface
 * @since 4.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Stepper Motor micro-step resolution options
 */
enum stepper_drv_micro_step_resolution {
	/** Full step resolution */
	STEPPER_DRV_MICRO_STEP_1 = 1,
	/** 2 micro-steps per full step */
	STEPPER_DRV_MICRO_STEP_2 = 2,
	/** 4 micro-steps per full step */
	STEPPER_DRV_MICRO_STEP_4 = 4,
	/** 8 micro-steps per full step */
	STEPPER_DRV_MICRO_STEP_8 = 8,
	/** 16 micro-steps per full step */
	STEPPER_DRV_MICRO_STEP_16 = 16,
	/** 32 micro-steps per full step */
	STEPPER_DRV_MICRO_STEP_32 = 32,
	/** 64 micro-steps per full step */
	STEPPER_DRV_MICRO_STEP_64 = 64,
	/** 128 micro-steps per full step */
	STEPPER_DRV_MICRO_STEP_128 = 128,
	/** 256 micro-steps per full step */
	STEPPER_DRV_MICRO_STEP_256 = 256,
};

/**
 * @brief Macro to calculate the index of the microstep resolution
 * @param res Microstep resolution
 */
#define MICRO_STEP_RES_INDEX(res) LOG2(res)

#define VALID_MICRO_STEP_RES(res)                                                                  \
	((res) == STEPPER_DRV_MICRO_STEP_1 || (res) == STEPPER_DRV_MICRO_STEP_2 ||                 \
	 (res) == STEPPER_DRV_MICRO_STEP_4 || (res) == STEPPER_DRV_MICRO_STEP_8 ||                 \
	 (res) == STEPPER_DRV_MICRO_STEP_16 || (res) == STEPPER_DRV_MICRO_STEP_32 ||               \
	 (res) == STEPPER_DRV_MICRO_STEP_64 || (res) == STEPPER_DRV_MICRO_STEP_128 ||              \
	 (res) == STEPPER_DRV_MICRO_STEP_256)

enum stepper_drv_event {
	/** Stepper driver stall detected */
	STEPPER_DRV_EVENT_STALL_DETETCTED = 0,
	/** Stepper driver fault detected */
	STEPPER_DRV_EVENT_FAULT_DETECTED = 1,
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Stepper Drv driver API definition and system call entry points.
 *
 */

/**
 * @brief Enable the stepper driver
 *
 * @see stepper_drv_enable() for details.
 */
typedef int (*stepper_drv_enable_t)(const struct device *dev);

/**
 * @brief Disable the stepper driver
 *
 * @see stepper_drv_disable() for details.
 */
typedef int (*stepper_drv_disable_t)(const struct device *dev);

/**
 * @brief Set the stepper direction
 *
 * @see stepper_drv_set_direction() for details.
 */
typedef int (*stepper_drv_direction_t)(const struct device *dev,
				       const enum stepper_direction direction);

/**
 * @brief Do a step
 *
 * @see stepper_drv_step() for details.
 */
typedef int (*stepper_drv_step_t)(const struct device *dev);

/**
 * @brief Set the stepper micro-step resolution
 *
 * @see stepper_drv_set_micro_step_res() for details.
 */
typedef int (*stepper_drv_set_micro_step_res_t)(
	const struct device *dev, const enum stepper_drv_micro_step_resolution resolution);

/**
 * @brief Get the stepper micro-step resolution
 *
 * @see stepper_drv_get_micro_step_res() for details.
 */
typedef int (*stepper_drv_get_micro_step_res_t)(const struct device *dev,
						enum stepper_drv_micro_step_resolution *resolution);

/**
 * @brief Callback function for stepper driver events
 */
typedef void (*stepper_drv_event_cb_t)(const struct device *dev, const enum stepper_drv_event event,
				       void *user_data);

/**
 * @brief Set the callback function to be called when a stepper_drv_event occurs
 *
 * @see stepper_drv_set_event_callback() for details.
 */
typedef int (*stepper_drv_set_event_callback_t)(const struct device *dev,
						stepper_drv_event_cb_t callback, void *user_data);

/**
 * @brief Stepper DRV Driver API
 */
__subsystem struct stepper_drv_driver_api {
	stepper_drv_enable_t enable;
	stepper_drv_disable_t disable;
	stepper_drv_direction_t set_direction;
	stepper_drv_step_t step;
	stepper_drv_set_micro_step_res_t set_micro_step_res;
	stepper_drv_get_micro_step_res_t get_micro_step_res;
	stepper_drv_set_event_callback_t set_event_cb;
};

/**
 * @endcond
 */

/**
 * @brief Enable stepper driver
 *
 * @details Enabling the driver shall switch on the power stage and energize the coils.
 *
 * @param dev pointer to the stepper_drv driver instance
 *
 * @retval -EIO Error during Enabling
 * @retval 0 Success
 */
__syscall int stepper_drv_enable(const struct device *dev);

static inline int z_impl_stepper_drv_enable(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_drv_driver_api *api = (const struct stepper_drv_driver_api *)dev->api;

	return api->enable(dev);
}

/**
 * @brief Disable stepper driver
 *
 * @details Disabling the driver shall switch off the power stage and de-energize the coils.
 *
 * @param dev pointer to the stepper_drv driver instance
 *
 * @retval  -ENOTSUP Disabling of driver is not supported.
 * @retval -EIO Error during Disabling
 * @retval 0 Success
 */
__syscall int stepper_drv_disable(const struct device *dev);

static inline int z_impl_stepper_drv_disable(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_drv_driver_api *api = (const struct stepper_drv_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Do a step.
 * @details This function will cause the stepper to do one micro-step.
 * This function is typically used in stepper_drv stepper drivers where
 * an external stepper motion controller is used to control the stepper.
 * This function will not increment/decrement any position related data. Doing so
 * is responsibility of the caller (e.g. stepper motion controller).
 *
 * @param dev pointer to the stepper_drv driver instance
 *
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
__syscall int stepper_drv_step(const struct device *dev);

static inline int z_impl_stepper_drv_step(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_drv_driver_api *api = (const struct stepper_drv_driver_api *)dev->api;

	return api->step(dev);
}

/**
 * @brief Set the stepper direction
 * @details This function sets the direction of the stepper motor.
 *
 * @param dev pointer to the stepper_drv driver instance
 * @param direction The direction to set
 *
 * @retval -EINVAL If the requested direction is invalid
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
__syscall int stepper_drv_set_direction(const struct device *dev,
					const enum stepper_direction direction);

static inline int z_impl_stepper_drv_set_direction(const struct device *dev,
						   const enum stepper_direction direction)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_drv_driver_api *api = (const struct stepper_drv_driver_api *)dev->api;

	if ((direction != STEPPER_DIRECTION_POSITIVE) &&
	    (direction != STEPPER_DIRECTION_NEGATIVE)) {
		return -EINVAL;
	}

	return api->set_direction(dev, direction);
}
/**
 * @brief Set the micro-step resolution in stepper driver
 *
 * @param dev pointer to the step dir driver instance
 * @param res micro-step resolution
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval -EINVAL If the requested resolution is invalid
 * @retval -ENOTSUP If the requested resolution is not supported
 * @retval 0 Success
 */
__syscall int stepper_drv_set_micro_step_res(const struct device *dev,
					     enum stepper_drv_micro_step_resolution res);

static inline int z_impl_stepper_drv_set_micro_step_res(const struct device *dev,
							enum stepper_drv_micro_step_resolution res)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_drv_driver_api *api = (const struct stepper_drv_driver_api *)dev->api;

	if (api->set_micro_step_res == NULL) {
		return -ENOSYS;
	}

	if (!VALID_MICRO_STEP_RES(res)) {
		return -EINVAL;
	}
	return api->set_micro_step_res(dev, res);
}

/**
 * @brief Get the micro-step resolution in stepper driver
 *
 * @param dev pointer to the stepper_drv driver instance
 * @param res micro-step resolution
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_drv_get_micro_step_res(const struct device *dev,
					     enum stepper_drv_micro_step_resolution *res);

static inline int z_impl_stepper_drv_get_micro_step_res(const struct device *dev,
							enum stepper_drv_micro_step_resolution *res)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(res != NULL);
	const struct stepper_drv_driver_api *api = (const struct stepper_drv_driver_api *)dev->api;

	return api->get_micro_step_res(dev, res);
}

/**
 * @brief Set the callback function to be called when a stepper_drv_event occurs
 *
 * @param dev pointer to the stepper_drv driver instance
 * @param callback Callback function to be called when a stepper_drv_event occurs
 * passing NULL will disable the callback
 * @param user_data User data to be passed to the callback function
 *
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_drv_set_event_cb(const struct device *dev, stepper_drv_event_cb_t callback,
				       void *user_data);

static inline int z_impl_stepper_drv_set_event_cb(const struct device *dev,
						  stepper_drv_event_cb_t cb, void *user_data)
{
	__ASSERT_NO_MSG(dev != NULL);
	const struct stepper_drv_driver_api *api = (const struct stepper_drv_driver_api *)dev->api;

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

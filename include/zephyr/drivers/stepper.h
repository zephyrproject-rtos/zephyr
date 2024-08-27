/**
 * @file drivers/stepper.h
 *
 * @brief Public API for Stepper Driver
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_

/**
 * @brief Stepper Motor Controller Interface
 * @defgroup stepper_interface Stepper Motor Controller Interface
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
 * @brief Stepper Motor micro step resolution options
 */
enum micro_step_resolution {
	/** Full step resolution */
	STEPPER_FULL_STEP = 1,
	/** 2 micro steps per full step */
	STEPPER_MICRO_STEP_2 = 2,
	/** 4 micro steps per full step */
	STEPPER_MICRO_STEP_4 = 4,
	/** 8 micro steps per full step */
	STEPPER_MICRO_STEP_8 = 8,
	/** 16 micro steps per full step */
	STEPPER_MICRO_STEP_16 = 16,
	/** 32 micro steps per full step */
	STEPPER_MICRO_STEP_32 = 32,
	/** 64 micro steps per full step */
	STEPPER_MICRO_STEP_64 = 64,
	/** 128 micro steps per full step */
	STEPPER_MICRO_STEP_128 = 128,
	/** 256 micro steps per full step */
	STEPPER_MICRO_STEP_256 = 256,
};

/**
 * @brief Stepper Motor direction options
 */
enum stepper_direction {
	/** Positive direction */
	STEPPER_DIRECTION_POSITIVE = 0,
	/** Negative direction */
	STEPPER_DIRECTION_NEGATIVE,
};

/**
 * @brief Stepper Motor run mode options
 */
enum stepper_run_mode {
	/** Hold Mode */
	STEPPER_HOLD_MODE = 0,
	/** Position Mode*/
	STEPPER_POSITION_MODE,
	/** Velocity Mode */
	STEPPER_VELOCITY_MODE,
};

/**
 * @brief Stepper Motor signal results
 */
enum stepper_signal_result {
	/** Steps set using move or set_target_position have been executed */
	STEPPER_SIGNAL_STEPS_COMPLETED = 0,
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Stepper motor controller driver API definition and system call entry points.
 *
 */

/**
 * @brief enable or disable the stepper motor controller.
 *
 * @see stepper_enable() for details.
 */
typedef int (*stepper_enable_t)(const struct device *dev, const bool enable);

/**
 * @brief Move the stepper motor by a given number of micro_steps.
 *
 * @see stepper_move() for details.
 */
typedef int (*stepper_move_t)(const struct device *dev, const int32_t micro_steps,
			      struct k_poll_signal *async);

/**
 * @brief Set the max velocity in micro_steps per seconds.
 *
 * @see stepper_set_max_velocity() for details.
 */
typedef int (*stepper_set_max_velocity_t)(const struct device *dev,
					  const uint32_t micro_steps_per_second);

/**
 * @brief Set the micro-step resolution
 *
 * @see stepper_set_micro_step_res() for details.
 */
typedef int (*stepper_set_micro_step_res_t)(const struct device *dev,
					    const enum micro_step_resolution resolution);

/**
 * @brief Get the micro-step resolution
 *
 * @see stepper_get_micro_step_res() for details.
 */
typedef int (*stepper_get_micro_step_res_t)(const struct device *dev,
					    enum micro_step_resolution *resolution);
/**
 * @brief Set the actual a.k.a reference position of the stepper
 *
 * @see stepper_set_actual_position() for details.
 */
typedef int (*stepper_set_actual_position_t)(const struct device *dev, const int32_t value);

/**
 * @brief Get the actual a.k.a reference position of the stepper
 *
 * @see stepper_get_actual_position() for details.
 */
typedef int (*stepper_get_actual_position_t)(const struct device *dev, int32_t *value);

/**
 * @brief Set the absolute target position of the stepper
 *
 * @see stepper_set_target_position() for details.
 */
typedef int (*stepper_set_target_position_t)(const struct device *dev, const int32_t value,
					     struct k_poll_signal *async);

/**
 * @brief Is the target position fo the stepper reached
 *
 * @see stepper_is_moving() for details.
 */
typedef int (*stepper_is_moving_t)(const struct device *dev, bool *is_moving);

/**
 * @brief Enable constant velocity mode for the stepper with a given velocity
 *
 * @see stepper_enable_constant_velocity_mode() for details.
 */
typedef int (*stepper_enable_constant_velocity_mode_t)(const struct device *dev,
						       const enum stepper_direction direction,
						       const uint32_t value);

/**
 * @brief Stepper Motor Controller API
 */
__subsystem struct stepper_driver_api {
	stepper_enable_t enable;
	stepper_move_t move;
	stepper_set_max_velocity_t set_max_velocity;
	stepper_set_micro_step_res_t set_micro_step_res;
	stepper_get_micro_step_res_t get_micro_step_res;
	stepper_set_actual_position_t set_actual_position;
	stepper_get_actual_position_t get_actual_position;
	stepper_set_target_position_t set_target_position;
	stepper_is_moving_t is_moving;
	stepper_enable_constant_velocity_mode_t enable_constant_velocity_mode;
};

/**
 * @endcond
 */

/**
 * @brief Enable or Disable Motor Controller
 *
 * @param dev pointer to the stepper motor controller instance
 * @param enable Input enable or disable motor controller
 *
 * @retval -EIO Error during Enabling
 * @retval 0 Success
 */
__syscall int stepper_enable(const struct device *dev, const bool enable);

static inline int z_impl_stepper_enable(const struct device *dev, const bool enable)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->enable(dev, enable);
}

/**
 * @brief Set the micro_steps to be moved from the current position i.e. relative movement
 *
 * @param dev pointer to the stepper motor controller instance
 * @param micro_steps target micro_steps to be moved from the current position
 * @param async Pointer to a valid and ready to be signaled struct
 *              k_poll_signal. (Note: if NULL this function will not notify
 *              the end of the transaction, and whether it went successfully
 *              or not).
 *
 * @retval -EIO General input / output error
 * @retval	0 Success
 */
__syscall int stepper_move(const struct device *dev, int32_t micro_steps,
			   struct k_poll_signal *async);

static inline int z_impl_stepper_move(const struct device *dev, const int32_t micro_steps,
				      struct k_poll_signal *async)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->move(dev, micro_steps, async);
}

/**
 * @brief Set the target velocity to be reached by the motor
 *
 * @details For controllers such as DRV8825 where you
 * toggle the STEP Pin, the pulse_length would have to be calculated based on this parameter in the
 * driver. For controllers where velocity can be set, this parameter corresponds to max_velocity
 * @note Setting max velocity does not set the motor into motion, a combination of set_max_velocity
 * and move is required to set the motor into motion.
 *
 * @param dev pointer to the stepper motor controller instance
 * @param micro_steps_per_second speed in micro_steps per second
 *
 * @retval -EIO General input / output error
 * @retval -EINVAL If the requested velocity is not supported
 * @retval 0 Success
 */
__syscall int stepper_set_max_velocity(const struct device *dev, uint32_t micro_steps_per_second);

static inline int z_impl_stepper_set_max_velocity(const struct device *dev,
						  const uint32_t micro_steps_per_second)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->set_max_velocity(dev, micro_steps_per_second);
}

/**
 * @brief Set the microstep resolution in stepper motor controller
 *
 * @param dev pointer to the stepper motor controller instance
 * @param resolution microstep resolution
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval -ENOTSUP If the requested resolution is not supported
 * @retval 0 Success
 */
__syscall int stepper_set_micro_step_res(const struct device *dev,
					 enum micro_step_resolution resolution);

static inline int z_impl_stepper_set_micro_step_res(const struct device *dev,
						    enum micro_step_resolution resolution)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_micro_step_res == NULL) {
		return -ENOSYS;
	}
	return api->set_micro_step_res(dev, resolution);
}

/**
 * @brief Get the microstep resolution in stepper motor controller
 *
 * @param dev pointer to the stepper motor controller instance
 * @param resolution microstep resolution
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_get_micro_step_res(const struct device *dev,
					 enum micro_step_resolution *resolution);

static inline int z_impl_stepper_get_micro_step_res(const struct device *dev,
						    enum micro_step_resolution *resolution)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->get_micro_step_res == NULL) {
		return -ENOSYS;
	}
	return api->get_micro_step_res(dev, resolution);
}

/**
 * @brief Set the actual a.k.a reference position of the stepper
 *
 * @param dev Pointer to the stepper motor controller instance.
 * @param value The reference position to set in micro-steps.
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_set_actual_position(const struct device *dev, int32_t value);

static inline int z_impl_stepper_set_actual_position(const struct device *dev, const int32_t value)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_actual_position == NULL) {
		return -ENOSYS;
	}
	return api->set_actual_position(dev, value);
}

/**
 * @brief Get the actual a.k.a reference position of the stepper
 *
 * @param dev pointer to the stepper motor controller instance
 * @param value The actual position to get in micro_steps
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_get_actual_position(const struct device *dev, int32_t *value);

static inline int z_impl_stepper_get_actual_position(const struct device *dev, int32_t *value)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->get_actual_position == NULL) {
		return -ENOSYS;
	}
	return api->get_actual_position(dev, value);
}

/**
 * @brief Set the absolute target position of the stepper
 *
 * @param dev pointer to the stepper motor controller instance
 * @param value target position to set in micro_steps
 * @param async Pointer to a valid and ready to be signaled struct
 *              k_poll_signal. If changing the target position
 *              triggers stepper movement, this can be used to await
 *              the end of the transaction. (Note: can be left NULL)
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_set_target_position(const struct device *dev, int32_t value,
					  struct k_poll_signal *async);

static inline int z_impl_stepper_set_target_position(const struct device *dev, const int32_t value,
						     struct k_poll_signal *async)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_target_position == NULL) {
		return -ENOSYS;
	}
	return api->set_target_position(dev, value, async);
}

/**
 * @brief Check if the stepper motor is currently moving
 *
 * @param dev pointer to the stepper motor controller instance
 * @param is_moving Pointer to a boolean to store the moving status of the stepper motor
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_is_moving(const struct device *dev, bool *is_moving);

static inline int z_impl_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->is_moving == NULL) {
		return -ENOSYS;
	}
	return api->is_moving(dev, is_moving);
}

/**
 * @brief Enable constant velocity mode for the stepper with a given velocity
 *
 * @details activate constant velocity mode with the given velocity in micro_steps_per_second.
 * If velocity > 0, motor shall be set into motion and run incessantly until and unless stalled or
 * stopped using some other command, for instance, motor_enable(false).
 *
 * @param dev pointer to the stepper motor controller instance
 * @param direction The direction to set
 * @param value The velocity to set in steps per second where one step is dependent on the current
 * microstepping resolution:
 * > 0: Enable constant velocity mode with the given velocity in a given direction
 * 0: Disable constant velocity mode
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_enable_constant_velocity_mode(const struct device *dev,
						    enum stepper_direction direction,
						    uint32_t value);

static inline int z_impl_stepper_enable_constant_velocity_mode(
	const struct device *dev, const enum stepper_direction direction, const uint32_t value)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->enable_constant_velocity_mode == NULL) {
		return -ENOSYS;
	}
	return api->enable_constant_velocity_mode(dev, direction, value);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/stepper.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_ */

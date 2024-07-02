/**
 * @file
 *
 * @brief Public API for Stepper Motor Controller
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_STEPPER_H
#define ZEPHYR_INCLUDE_STEPPER_H

/**
 * @brief Stepper Motor Controller Interface
 * @defgroup stepper_interface Stepper Motor Controller Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/kernel.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stepper Motor micro step resolution options
 */
enum micro_step_resolution {
	STEPPER_FULL_STEP,
	STEPPER_MICRO_STEP_2,
	STEPPER_MICRO_STEP_4,
	STEPPER_MICRO_STEP_8,
	STEPPER_MICRO_STEP_16,
	STEPPER_MICRO_STEP_32,
	STEPPER_MICRO_STEP_64,
	STEPPER_MICRO_STEP_128,
	STEPPER_MICRO_STEP_256,
};

enum stepper_direction {
	STEPPER_DIRECTION_POSITIVE,
	STEPPER_DIRECTION_NEGATIVE,
};

/**
 * @brief enable or disable the stepper motor controller.
 *
 * See stepper_enable() for details.
 */
typedef int32_t (*stepper_enable_t)(const struct device *dev, bool enable);

/**
 * @brief Move the stepper motor by a given number of micro_steps.
 *
 * See stepper_move() for details.
 */
typedef int32_t (*stepper_move_t)(const struct device *dev, int32_t micro_steps);

/**
 * @brief Set the max velocity in micro_steps per seconds.
 *
 * See stepper_set_max_velocity() for details.
 */
typedef int32_t (*stepper_set_max_velocity_t)(const struct device *dev,
					      uint32_t micro_steps_per_second);

/**
 * @brief Set the micro-step resolution
 *
 * See stepper_set_micro_step_res() for details.
 */
typedef int32_t (*stepper_set_micro_step_res_t)(const struct device *dev,
						enum micro_step_resolution resolution);

/**
 * @brief Get the micro-step resolution
 *
 * See stepper_get_micro_step_res() for details.
 */
typedef int32_t (*stepper_get_micro_step_res_t)(const struct device *dev,
						enum micro_step_resolution *resolution);
/**
 * @brief Set the actual a.k.a reference position of the stepper
 *
 * See stepper_set_actual_position() for details.
 */
typedef int32_t (*stepper_set_actual_position_t)(const struct device *dev, const int32_t value);

/**
 * @brief Get the actual a.k.a reference position of the stepper
 *
 * See stepper_get_actual_position() for details.
 */
typedef int32_t (*stepper_get_actual_position_t)(const struct device *dev, int32_t *value);

/**
 * @brief Set the absolute target position of the stepper
 *
 * See stepper_set_target_position() for details.
 */
typedef int32_t (*stepper_set_target_position_t)(const struct device *dev, const int32_t value);

/**
 * @brief Is the target position fo the stepper reached
 *
 * See stepper_is_moving() for details.
 */
typedef int32_t (*stepper_is_moving_t)(const struct device *dev, bool *is_moving);

/**
 * @brief Enable constant velocity mode for the stepper with a given velocity
 *
 * See stepper_enable_constant_velocity_mode() for details.
 */
typedef int32_t (*stepper_enable_constant_velocity_mode_t)(const struct device *dev,
							   const enum stepper_direction direction,
							   const uint32_t value);

/**
 * @brief Stepper Motor Controller API
 */
__subsystem struct stepper_api {
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
 * @brief Enable or Disable Motor Controller
 * @param dev pointer to the stepper motor controller instance
 * @param enable Input enable or disable motor controller
 * @retval <0 Error during Enabling
 * @retval  0 Success
 */
__syscall int32_t stepper_enable(const struct device *dev, bool enable);

static inline int32_t z_impl_stepper_enable(const struct device *dev, bool enable)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	return api->enable(dev, enable);
}

/**
 * @brief Set the micro_steps to be moved from the current position i.e. relative movement
 * @param dev pointer to the stepper motor controller instance
 * @param micro_steps target micro_steps to be moved from the current position
 * @retval <0 Error during Enabling
 * @retval  0 Success
 */
__syscall int32_t stepper_move(const struct device *dev, int32_t micro_steps);

static inline int32_t z_impl_stepper_move(const struct device *dev, int32_t micro_steps)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	return api->move(dev, micro_steps);
}

/**
 * @brief Set the target velocity to be reached by the motor
 * @details For controllers such as DRV8825 where you
 * toggle the STEP Pin, the pulse_length would have to be calculated based on this parameter in the
 * driver. For controllers where velocity can be set, this parameter corresponds to max_velocity
 * @note Setting max velocity does not set the motor into motion, a combination of set_max_velocity
 * and move is required to set the motor into motion.
 * @param dev pointer to the stepper motor controller instance
 * @param micro_steps_per_second speed in micro_steps per second
 * @retval <0 Error during Enabling
 * @retval  0 Success
 */
__syscall int32_t stepper_set_max_velocity(const struct device *dev,
					   uint32_t micro_steps_per_second);

static inline int32_t z_impl_stepper_set_max_velocity(const struct device *dev,
						      uint32_t micro_steps_per_second)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	return api->set_max_velocity(dev, micro_steps_per_second);
}

/**
 * @brief Set the microstep resolution in stepper motor controller
 * @param dev pointer to the stepper motor controller instance
 * @param resolution microstep resolution
 * @retval -ENOSYS If not implemented by device driver
 * @retval       0 Success
 */
__syscall int32_t stepper_set_micro_step_res(const struct device *dev,
					     enum micro_step_resolution resolution);

static inline int32_t z_impl_stepper_set_micro_step_res(const struct device *dev,
							enum micro_step_resolution resolution)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	if (api->set_micro_step_res == NULL) {
		return -ENOSYS;
	}
	return api->set_micro_step_res(dev, resolution);
}

/**
 * @brief Get the microstep resolution in stepper motor controller
 * @param dev pointer to the stepper motor controller instance
 * @param resolution microstep resolution
 * @retval -ENOSYS If not implemented by device driver
 * @retval       0 Success
 */
__syscall int32_t stepper_get_micro_step_res(const struct device *dev,
					     enum micro_step_resolution *resolution);

static inline int32_t z_impl_stepper_get_micro_step_res(const struct device *dev,
							enum micro_step_resolution *resolution)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	if (api->get_micro_step_res == NULL) {
		return -ENOSYS;
	}
	return api->get_micro_step_res(dev, resolution);
}

/**
 * @brief Set the actual a.k.a reference position of the stepper
 * @param dev pointer to the stepper motor controller instance
 * @param value actual position to set in micro_steps.
 * @retval -ENOSYS If not implemented by device driver
 * @retval       0 Success
 */
__syscall int32_t stepper_set_actual_position(const struct device *dev, const int32_t value);

static inline int32_t z_impl_stepper_set_actual_position(const struct device *dev,
							 const int32_t value)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	if (api->set_actual_position == NULL) {
		return -ENOSYS;
	}
	return api->set_actual_position(dev, value);
}

/**
 * @brief Get the actual a.k.a reference position of the stepper
 * @param dev pointer to the stepper motor controller instance
 * @param value The actual position to get in micro_steps
 * @retval -ENOSYS If not implemented by device driver
 * @retval       0 Success
 */
__syscall int32_t stepper_get_actual_position(const struct device *dev, int32_t *value);

static inline int32_t z_impl_stepper_get_actual_position(const struct device *dev, int32_t *value)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	if (api->get_actual_position == NULL) {
		return -ENOSYS;
	}
	return api->get_actual_position(dev, value);
}

/**
 * @brief Set the absolute target position of the stepper
 * @param dev pointer to the stepper motor controller instance
 * @param value target position to set in micro_steps
 * @retval -ENOSYS If not implemented by device driver
 * @retval       0 Success
 */
__syscall int32_t stepper_set_target_position(const struct device *dev, const int32_t value);

static inline int32_t z_impl_stepper_set_target_position(const struct device *dev,
							 const int32_t value)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	if (api->set_target_position == NULL) {
		return -ENOSYS;
	}
	return api->set_target_position(dev, value);
}

/**
 * @brief Check if the stepper motor is currently moving
 * @param dev pointer to the stepper motor controller instance
 * @param is_moving Pointer to a boolean to store the moving status of the stepper motor
 * @retval -ENOSYS If not implemented by device driver
 * @retval       0 Success
 */
__syscall int32_t stepper_is_moving(const struct device *dev, bool *is_moving);

static inline int32_t z_impl_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

	if (api->is_moving == NULL) {
		return -ENOSYS;
	}
	return api->is_moving(dev, is_moving);
}

/**
 * @brief Enable constant velocity mode for the stepper with a given velocity
 * @details activate constant velocity mode with the given velocity in micro_steps_per_second.
 * If velocity > 0, motor shall be set into motion and run incessantly until and unless stalled or
 * stopped using some other command, for instance, motor_enable(false).
 * @param dev pointer to the stepper motor controller instance
 * @param direction The direction to set
 * @param value The velocity to set
 *		> 0: Enable constant velocity mode with the given velocity in a given direction
 *		  0: Disable constant velocity mode
 * @retval -ENOSYS If not implemented by device driver
 * @retval       0 Success
 */
__syscall int32_t stepper_enable_constant_velocity_mode(const struct device *dev,
							const enum stepper_direction direction,
							const uint32_t value);

static inline int32_t z_impl_stepper_enable_constant_velocity_mode(
	const struct device *dev, const enum stepper_direction direction, const uint32_t value)
{
	const struct stepper_api *api = (const struct stepper_api *)dev->api;

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

#include <syscalls/stepper.h>

#endif /* ZEPHYR_INCLUDE_STEPPER_H */

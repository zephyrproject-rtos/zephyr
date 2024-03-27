/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_STEPPER_MOTOR_CONTROLLER_H
#define ZEPHYR_INCLUDE_STEPPER_MOTOR_CONTROLLER_H

/** \addtogroup stepper_motor_controller
 *  @{
 */

/**
 * @file
 * @brief Public API for Stepper Motor Controller
 *
 */

#include <zephyr/kernel.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stepper Motor Position Format required while setting or getting stepper motor positions.
 */
enum micro_step_resolution {
	FULL_STEP,
	MICRO_STEP_2,
	MICRO_STEP_4,
	MICRO_STEP_8,
	MICRO_STEP_16,
	MICRO_STEP_32,
	MICRO_STEP_64,
	MICRO_STEP_128,
	MICRO_STEP_256,
};

/**
 * @typedef stepper_motor_controller_enable_t
 * @brief enable or disable the stepper motor controller.
 */
typedef int32_t (*stepper_motor_controller_enable_t)(const struct device *dev, bool enable);

/**
 * @typedef stepper_motor_controller_set_steps_to_move_t
 * @brief Stepper Motor Controller shall move the motor with given steps
 */
typedef int32_t (*stepper_motor_controller_set_steps_to_move_t)(const struct device *dev,
								int32_t steps);

/**
 * @typedef stepper_motor_controller_set_velocity_t
 * @brief Set the velocity in steps per seconds. For controllers such as DRV8825 where you
 * toggle the STEP Pin, the pulse_length would have to be calculated based on this parameter in the
 * driver. For controllers where velocity can be set, this parameter corresponds to max_velocity
 */
typedef int32_t (*stepper_motor_controller_set_velocity_t)(const struct device *dev,
							   uint32_t steps_per_second);

/**
 * @typedef stepper_motor_controller_set_micro_step_res_t
 * @brief Set the microstep resolution
 */
typedef int32_t (*stepper_motor_controller_set_micro_step_res_t)(
	const struct device *dev, enum micro_step_resolution resolution);

/**
 * @typedef stepper_motor_controller_set_reference_t
 * @brief reset the actual position in a stepper motor controller
 */
typedef int32_t (*stepper_motor_controller_set_reference_t)(const struct device *dev,
							    int32_t position);

/**
 * @typedef stepper_motor_controller_get_position_t
 * @brief Get the actual position from stepper motor controller
 */
typedef int32_t (*stepper_motor_controller_get_position_t)(const struct device *dev,
							   int32_t *position);

/**
 * @typedef stepper_motor_controller_set_position_t
 * @brief Set the target position in stepper motor controller which should set stepper motor in
 * motion
 */
typedef int32_t (*stepper_motor_controller_set_position_t)(const struct device *dev,
							   int32_t position);

/**
 * @typedef stepper_motor_controller_enable_velocity_mode_t
 * @brief Stepper motor controller shall activate the mode where the motor reaches a constant
 * velocity and runs incessantly
 */
typedef int32_t (*stepper_motor_controller_enable_velocity_mode_t)(const struct device *dev,
								   int32_t steps_per_second);

/**
 * @typedef stepper_motor_controller_get_stall_status_t
 * @brief Get the status if a motor is stalled or not from stepper motor controller
 */
typedef int32_t (*stepper_motor_controller_get_stall_status_t)(const struct device *dev,
							       bool *stall_status);

/**
 * @typedef stepper_motor_controller_enable_stall_detection_t
 * @brief Enable or Disable stall detection
 */
typedef int32_t (*stepper_motor_controller_enable_stall_detection_t)(const struct device *dev,
								     bool enable_stall_detection);

/**
 * @typedef stall_detection_motor_controller_set_stall_detection_threshold_t
 * @brief Set the stall detection threshold
 */
typedef int32_t (*stall_detection_motor_controller_set_stall_detection_threshold_t)(
	const struct device *dev, int32_t stall_detection_threshold);

__subsystem struct stepper_motor_controller_api {
	stepper_motor_controller_enable_t stepper_motor_controller_enable;
	stepper_motor_controller_set_steps_to_move_t stepper_motor_controller_set_steps_to_move;
	stepper_motor_controller_set_velocity_t stepper_motor_controller_set_velocity;
	stepper_motor_controller_set_micro_step_res_t stepper_motor_controller_set_micro_step_res;
#ifdef CONFIG_STEPPER_MOTOR_CONTROLLER_HAS_CONSTANT_VELOCITY_MODE
	stepper_motor_controller_enable_velocity_mode_t
		stepper_motor_controller_enable_velocity_mode;
#endif /* CONFIG_STEPPER_MOTOR_CONTROLLER_HAS_CONSTANT_VELOCITY_MODE */
#ifdef CONFIG_STEPPER_MOTOR_CONTROLLER_HAS_STALL_DETECTION
	stepper_motor_controller_get_stall_status_t stepper_motor_controller_get_stall_status;

	stepper_motor_controller_enable_stall_detection_t
		stepper_motor_controller_enable_stall_detection;
	stall_detection_motor_controller_set_stall_detection_threshold_t
		stepper_motor_controller_set_stall_detection_threshold;
#endif /* CONFIG_STEPPER_MOTOR_CONTROLLER_HAS_STALL_DETECTION */
#ifdef CONFIG_STEPPER_MOTOR_CONTROLLER_HAS_ABSOLUTE_POSITIONING
	stepper_motor_controller_set_reference_t stepper_motor_controller_set_reference;
	stepper_motor_controller_get_position_t stepper_motor_controller_get_position;
	stepper_motor_controller_set_position_t stepper_motor_controller_set_position;
#endif /* CONFIG_STEPPER_MOTOR_CONTROLLER_HAS_ABSOLUTE_POSITIONING */
};

/**
 * @brief Enable or Disable Motor Controller
 * @param dev pointer to the stepper motor controller instance
 * @param enable Input enable or disable motor controller
 * @return <0 Error during Enabling
 *          0 Success
 */
__syscall int32_t stepper_motor_controller_enable(const struct device *dev, bool enable);

static inline int32_t z_impl_stepper_motor_controller_enable(const struct device *dev, bool enable)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	return api->stepper_motor_controller_enable(dev, enable);
}

/**
 * @brief Set the steps to be moved from the current position i.e. relative movement
 * @param dev pointer to the stepper motor controller instance
 * @param steps target steps to be moved from the current position
 * @return <0 Error during Enabling
 *          0 Success
 */
__syscall int32_t stepper_motor_controller_set_steps_to_move(const struct device *dev,
							     int32_t steps);

static inline int32_t z_impl_stepper_motor_controller_set_steps_to_move(const struct device *dev,
									int32_t steps)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	return api->stepper_motor_controller_set_steps_to_move(dev, steps);
}

/**
 * @brief Set the target velocity to be reached by the motor
 * @param dev pointer to the stepper motor controller instance
 * @param steps_per_second Enter the speed in steps per second
 * @return <0 Error during Enabling
 *          0 Success
 */
__syscall int32_t stepper_motor_controller_set_velocity(const struct device *dev,
							uint32_t steps_per_second);

static inline int32_t z_impl_stepper_motor_controller_set_velocity(const struct device *dev,
								   uint32_t steps_per_second)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	return api->stepper_motor_controller_set_velocity(dev, steps_per_second);
}

/**
 * @brief Set the microstep resolution in stepper motor controller
 * @param dev pointer to the stepper motor controller instance
 * @param resolution microstep resolution
 * @return <0 Error during Enabling
 *          0 Success
 */
__syscall int32_t stepper_motor_controller_set_micro_step_res(
	const struct device *dev, enum micro_step_resolution resolution);

static inline int32_t
z_impl_stepper_motor_controller_set_micro_step_res(const struct device *dev,
						   enum micro_step_resolution resolution)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	return api->stepper_motor_controller_set_micro_step_res(dev, resolution);
}

/**
 * @brief Enable velocity mode where motor reaches and runs with a constant velocity
 * @details Stepper motor controller shall activate the mode where the motor reaches a constant
 * velocity and runs incessantly
 * @param dev pointer to the stepper motor controller instance
 * @param steps_per_second > 0 --> run in positive direction
 *                 < 0 --> run in negative direction
 *                 = 0 --> disable constant velocity mode
 * @retval -ENOSYS If this function is not implemented by the driver.
 *               0 Success
 */
__syscall int32_t stepper_motor_controller_enable_velocity_mode(const struct device *dev,
								int32_t steps_per_second);

static inline int32_t z_impl_stepper_motor_controller_enable_velocity_mode(const struct device *dev,
									   int32_t steps_per_second)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	if (NULL == api->stepper_motor_controller_enable_velocity_mode) {
		return -ENOSYS;
	}
	return api->stepper_motor_controller_enable_velocity_mode(dev, steps_per_second);
}

/**
 * @brief Get stall status from the motor controller
 * @details Stepper motor controller shall check and respond if a motor stall is detected or not
 * @param dev pointer to the stepper motor controller instance
 * @param stall_status Output True if motor stalled is detected, false otherwise
 * @retval -ENOSYS If this function is not implemented by the driver.
 *               0 Success
 */
__syscall int32_t stepper_motor_controller_get_stall_status(const struct device *dev,
							    bool *stall_status);

static inline int32_t z_impl_stepper_motor_controller_get_stall_status(const struct device *dev,
								       bool *stall_status)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	if (NULL == api->stepper_motor_controller_get_stall_status) {
		return -ENOSYS;
	}
	return api->stepper_motor_controller_get_stall_status(dev, stall_status);
}

/**
 * @brief Enable or Disable stall detection
 * @param dev pointer to the stepper motor controller instance
 * @param enable_stall_detection Input True to enable stall detection, false otherwise
 * @retval -ENOSYS If this function is not implemented by the driver.
 *               0 Success
 */
__syscall int32_t stepper_motor_controller_enable_stall_detection(const struct device *dev,
								  bool enable_stall_detection);

static inline int32_t
z_impl_stepper_motor_controller_enable_stall_detection(const struct device *dev,
						       bool enable_stall_detection)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	if (NULL == api->stepper_motor_controller_enable_stall_detection) {
		return -ENOSYS;
	}
	return api->stepper_motor_controller_enable_stall_detection(dev, enable_stall_detection);
}

/**
 * @brief Set stall detection threshold
 * @details Configure sensitivity of stall detection
 * @param dev pointer to the stepper motor controller instance
 * @param stall_detection_threshold Input stall detection threshold
 * @retval -ENOSYS If this function is not implemented by the driver.
 *               0 Success
 */
__syscall int32_t stepper_motor_controller_set_stall_detection_threshold(
	const struct device *dev, int32_t stall_detection_threshold);

static inline int32_t
z_impl_stepper_motor_controller_set_stall_detection_threshold(const struct device *dev,
							      int32_t stall_detection_threshold)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	if (api->stepper_motor_controller_set_stall_detection_threshold) {
		return -ENOSYS;
	}
	return api->stepper_motor_controller_set_stall_detection_threshold(
		dev, stall_detection_threshold);
}

/**
 * @brief Set reference position within the controller
 * @details Set the reference of the actual position within the controller
 * @param dev pointer to the stepper motor controller instance
 * @param position Input enter the position in full-steps, for some controllers this position might
 * have to be
 * @retval -ENOSYS If this function is not implemented by the driver.
 *               0 Success
 */
__syscall int32_t stepper_motor_controller_set_reference(const struct device *dev,
							 int32_t position);

static inline int32_t z_impl_stepper_motor_controller_set_reference(const struct device *dev,
								    int32_t position)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	if (NULL == api->stepper_motor_controller_set_reference) {
		return -ENOSYS;
	}
	return api->stepper_motor_controller_set_reference(dev, position);
}

/**
 * @brief Get actual motor position from the controller
 * @param dev pointer to the stepper motor controller instance
 * @param position Output pointer to the actual position in full steps as retrieved from the
 * controller
 * @retval -ENOSYS If this function is not implemented by the driver.
 *               0 Success
 */
__syscall int32_t stepper_motor_controller_get_position(const struct device *dev,
							int32_t *position);

static inline int32_t z_impl_stepper_motor_controller_get_position(const struct device *dev,
								   int32_t *position)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	if (NULL == api->stepper_motor_controller_get_position) {
		return -ENOSYS;
	}
	return api->stepper_motor_controller_get_position(dev, position);
}

/**
 * @brief Set the absolute position in the controller
 * @param dev pointer to the stepper motor controller instance
 * @param position Input target position in full steps
 * @retval -ENOSYS If this function is not implemented by the driver.
 *               0 Success
 */
__syscall int32_t stepper_motor_controller_set_position(const struct device *dev, int32_t position);

static inline int32_t z_impl_stepper_motor_controller_set_position(const struct device *dev,
								   int32_t position)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	if (NULL == api->stepper_motor_controller_set_position) {
		return -ENOSYS;
	}
	return api->stepper_motor_controller_set_position(dev, position);
}

#ifdef __cplusplus
}
#endif

/** @}*/

#include <syscalls/stepper_motor_controller.h>

#endif /* ZEPHYR_INCLUDE_STEPPER_MOTOR_CONTROLLER_H */

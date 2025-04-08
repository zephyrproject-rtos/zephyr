/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov <mail@andrestefanov.de>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file stepper_motion_controller.c
 * @brief Stepper motor motion controller implementation
 *
 * This file implements a real-time stepper motor motion controller with optimized
 * timing signal handling. The critical timing path in
 * stepper_motion_controller_handle_timing_signal() is optimized for minimal latency and jitter by
 * inlining operations and using compiler branch prediction hints.
 */

#include "stepper_motion_controller.h"

#include <stdlib.h>
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_motion_controller, CONFIG_STEPPER_LOG_LEVEL);

/**
 * @brief Returns the sign of a number (-1, 0, or 1)
 * @param x The number to get the sign of
 * @return -1 if negative, 1 if positive or zero
 */
#define SIGN(x) ((x) < 0 ? -1 : 1)

/**
 * @brief Get motion controller config from device pointer
 * @param dev The device pointer
 * @return Pointer to stepper_motion_controller_config structure
 */
static inline const struct stepper_motion_controller_config *get_config(const struct device *dev)
{
	return (const struct stepper_motion_controller_config *)dev->config;
}

/**
 * @brief Get motion controller data from device pointer
 * @param dev The device pointer
 * @return Pointer to stepper_motion_controller_data structure
 */
static inline struct stepper_motion_controller_data *get_data(const struct device *dev)
{
	return (struct stepper_motion_controller_data *)dev->data;
}

/**
 * @brief Special position values for continuous movement mode in positive direction
 */
#define CONTINUOUS_POSITIVE_POSITION INT32_MAX

/**
 * @brief Special position values for continuous movement mode in negative direction
 */
#define CONTINUOUS_NEGATIVE_POSITION INT32_MIN

/**
 * @brief Check if position indicates continuous movement mode
 * @param pos Position value to check
 * @return true if position indicates continuous movement
 */
#define IS_CONTINUOUS_MOVEMENT(pos)                                                                \
	((pos) == CONTINUOUS_POSITIVE_POSITION || (pos) == CONTINUOUS_NEGATIVE_POSITION)

/**
 * @brief Set the direction of the stepper motor
 *
 * This function sets the direction of the stepper motor and updates the internal state.
 * It is inlined for performance reasons, as it is called frequently during motion control.
 *
 * @param dev The stepper device
 * @param config Preloaded controller config (optimization for interrupt context)
 * @param data Preloaded controller data (optimization for interrupt context)
 * @param direction The desired direction to set
 */
static inline void stepper_motion_controller_set_direction(const struct device *dev,
							const struct stepper_motion_controller_config *config,
							struct stepper_motion_controller_data *data,
						    const enum stepper_direction direction)
{
	config->callbacks->set_direction(dev, direction);

	data->direction = direction;
	LOG_DBG("Direction set to %d", direction);
}

/**
 * @brief Inline timing interval calculation and movement continuation logic
 *
 * This inline function handles the core logic for:
 * - Continuing current movement with next interval
 * - Stopping movement when ramp completes  
 * - Starting queued movement if target position pending
 * - Triggering completion event when all movement finished
 *
 * PERFORMANCE NOTES:
 * - Marked inline for zero function call overhead in critical timing path
 * - Used by both timing signal handler (interrupt context) and non-critical paths
 * - Branch prediction hints always optimize for movement continuation (common case)
 * - Error handling uses unlikely() hints for exceptional cases
 *
 * @param dev The stepper device
 * @param config Preloaded controller config (optimization for interrupt context)
 * @param data Preloaded controller data (optimization for interrupt context)
 */
static inline void stepper_motion_controller_handle_next_interval(
	const struct device *dev,
	const struct stepper_motion_controller_config *config,
	struct stepper_motion_controller_data *data)
{
	const uint64_t next_interval = stepper_ramp_get_next_interval(&data->ramp);

	if (likely(next_interval > 0)) {
		/* COMMON CASE: Continue movement - next interval calculated */
		const int ret = stepper_timing_source_start(config->timing_source, next_interval);
		if (unlikely(ret < 0)) {
			LOG_ERR("Failed to start timing source: %d", ret);
		}
	} else {
		/* UNCOMMON CASE: Current ramp sequence finished - stop timing source */
		const int ret = stepper_timing_source_stop(config->timing_source);
		if (unlikely(ret < 0)) {
			LOG_ERR("Failed to stop timing source: %d", ret);
		}

		/* Check if there's a queued movement to start */
		const bool has_queued_move = (data->target_position != data->position && 
					      !IS_CONTINUOUS_MOVEMENT(data->target_position));
		
		if (unlikely(has_queued_move)) {
			/* RARE CASE: Queue next move - inline for performance */
			LOG_DBG("Starting queued movement to position %d", data->target_position);

			stepper_motion_controller_set_direction(
				dev, config, data, SIGN(data->target_position - data->position));

			stepper_ramp_prepare_move(&data->ramp, abs(data->target_position - data->position));

			const uint64_t new_interval = stepper_ramp_get_next_interval(&data->ramp);
			const int start_ret =
				stepper_timing_source_start(config->timing_source, new_interval);
			if (unlikely(start_ret < 0)) {
				LOG_ERR("Failed to start timing source for queued move: %d", start_ret);
			}
		} else {
			/* FINAL CASE: All movement completed - notify completion */
			LOG_DBG("Motion completed");
			config->callbacks->event(dev, STEPPER_EVENT_STEPS_COMPLETED);
		}
	}
}

/**
 * @brief Real-time timing signal handler (PERFORMANCE CRITICAL)
 *
 * This function is called from interrupt context and must execute with minimal
 * latency and jitter. All critical operations are inlined to avoid function
 * call overhead. Compiler branch prediction hints (likely/unlikely) optimize
 * for the common execution paths.
 *
 * RACE CONDITION PROTECTION:
 * - Uses spinlock to protect against concurrent access to motion data
 * - Critical section is kept minimal to reduce interrupt latency
 * - Spinlock protects position updates and timing calculations
 *
 * OPTIMIZATION NOTES:
 * - Step execution is inlined to avoid function call overhead
 * - Position tracking is inlined
 * - Timing calculations are inlined to reduce call stack depth
 * - Branch prediction hints optimize for movement continuation (common case)
 * - Error handling uses unlikely() hints for exceptional cases
 *
 * @param user_data Pointer to the stepper device (passed from timing source)
 */
void stepper_motion_controller_handle_timing_signal(const void *user_data)
{
	const struct device *dev = user_data;
	const struct stepper_motion_controller_config *config = get_config(dev);
	struct stepper_motion_controller_data *data = get_data(dev);

	/* INLINE STEP EXECUTION - avoid function call overhead in critical path */
	config->callbacks->step(dev);

	/* CRITICAL SECTION: Protect position tracking and timing calculations from race conditions */
	K_SPINLOCK(&data->lock) {
		/* INLINE POSITION TRACKING - minimize memory access patterns */
		data->position += data->direction;

		/* INLINE TIMING CALCULATION - use optimized inline helper with branch prediction hints */
		stepper_motion_controller_handle_next_interval(dev, config, data);
	}
}

/**
 * @brief Initialize the stepper motion controller
 *
 * Sets up the timing source callback, initializes default direction,
 * and configures the default ramp profile.
 *
 * @param dev The stepper device to initialize
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_init(const struct device *dev)
{
	const struct stepper_motion_controller_config *config = get_config(dev);
	struct stepper_motion_controller_data *data = get_data(dev);

	/* Validate callback structure */
	if (config->callbacks == NULL || config->callbacks->step == NULL ||
	    config->callbacks->set_direction == NULL || config->callbacks->event == NULL) {
		LOG_ERR("Invalid or incomplete callback configuration");
		return -EINVAL;
	}

	/* Configure timing source callback to our optimized handler */
	config->timing_source->callback = &stepper_motion_controller_handle_timing_signal;
	config->timing_source->user_data = dev;

	/* Set initial direction */
	stepper_motion_controller_set_direction(dev, config, data, STEPPER_DIRECTION_POSITIVE);

	/* Initialize timing source hardware */
	const int ret = stepper_timing_source_init(config->timing_source);
	if (ret < 0) {
		LOG_ERR("Failed to initialize timing source: %d", ret);
		return ret;
	}

	/* Set default ramp profile (square wave with zero interval) */
	data->ramp.profile.type = STEPPER_RAMP_TYPE_SQUARE;
	data->ramp.profile.square.interval_ns = 0;

	/* Initialize position tracking */
	data->position = 0;
	data->target_position = 0;

	LOG_INF("Stepper motion controller initialized successfully");
	return 0;
}

/**
 * @brief Move the stepper motor by a relative number of micro steps
 *
 * This function calculates the absolute target position and delegates to move_to()
 * to handle all movement scenarios and optimizations.
 *
 * @param dev The stepper device
 * @param micro_steps Number of micro steps to move (positive or negative)
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_move_by(const struct device *dev, const int32_t micro_steps)
{
	struct stepper_motion_controller_data *data = get_data(dev);

	LOG_DBG("Move request: %d microsteps", micro_steps);

	/* Calculate absolute target position and delegate to move_to */
	const int32_t target_position = data->position + micro_steps;
	return stepper_motion_controller_move_to(dev, target_position);
}

/**
 * @brief Check if the stepper motor is currently moving
 * @param dev The stepper device
 * @param is_moving Pointer to store the movement status
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_is_moving(const struct device *dev, bool *is_moving)
{
	if (is_moving == NULL) {
		return -EINVAL;
	}

	const struct stepper_motion_controller_data *data = get_data(dev);

	*is_moving = (data->target_position != data->position) || IS_CONTINUOUS_MOVEMENT(data->target_position);
	return 0;
}

/**
 * @brief Set the ramp profile for stepper acceleration/deceleration
 * @param dev The stepper device
 * @param ramp Pointer to the ramp profile configuration
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_set_ramp(const struct device *dev,
				       const struct stepper_ramp_profile *ramp)
{
	if (ramp == NULL) {
		return -EINVAL;
	}

	struct stepper_motion_controller_data *data = get_data(dev);

	K_SPINLOCK(&data->lock) {
		data->ramp.profile = *ramp;
		LOG_DBG("Ramp profile updated to type %d", ramp->type);
	}

	return 0;
}

/**
 * @brief Stop the stepper motor with deceleration ramp
 *
 * The motor will decelerate according to the current ramp profile.
 * If the motor is already stopped, this function has no effect.
 *
 * @param dev The stepper device
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_stop(const struct device *dev)
{
	const struct stepper_motion_controller_config *config = get_config(dev);
	struct stepper_motion_controller_data *data = get_data(dev);

	LOG_DBG("Stop requested");

	const uint64_t stop_steps_count = stepper_ramp_prepare_stop(&data->ramp);

	if (stop_steps_count > 0) {
		/* Motor needs deceleration steps to stop smoothly */
		data->target_position = data->position + (SIGN(data->direction) * stop_steps_count);
		LOG_DBG("Deceleration requires %llu steps", stop_steps_count);

		stepper_motion_controller_handle_next_interval(dev, config, data);
	} else {
		/* Motor can stop immediately */
		data->target_position = data->position;

		const int ret = stepper_timing_source_stop(config->timing_source);
		if (ret < 0) {
			LOG_ERR("Failed to stop timing source: %d", ret);
			return ret;
		}

		LOG_DBG("Motor stopped immediately");
	}

	return 0;
}

/**
 * @brief Move the stepper motor to an absolute position
 * @param dev The stepper device
 * @param position The target absolute position in micro steps
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_move_to(const struct device *dev, const int32_t position)
{
	struct stepper_motion_controller_data *data = get_data(dev);
	const struct stepper_motion_controller_config *config = get_config(dev);

	K_SPINLOCK(&data->lock) {
		LOG_DBG("Move to position %d (current: %d)", position, data->position);

		const int32_t relative_steps = position - data->position;
		if (relative_steps == 0) {
			/* Already at target position */
			LOG_DBG("Already at target position - signaling completion");
			config->callbacks->event(dev, STEPPER_EVENT_STEPS_COMPLETED);
			return 0;
		}

		uint64_t movement_steps_count = 0;
		const bool is_moving =
			stepper_timing_source_get_interval(config->timing_source) > 0;
		const bool is_same_direction = (data->direction == SIGN(relative_steps));

		if (is_moving && !is_same_direction) {
			/*
			 * Motor is moving in opposite direction - must decelerate to stop
			 * before starting movement in the new direction
			 */
			LOG_DBG("Direction change detected - preparing deceleration stop");
			movement_steps_count = stepper_ramp_prepare_stop(&data->ramp);
		}

		if (movement_steps_count == 0) {
			/* Motor is stopped or moving in same direction - start new move */
			stepper_motion_controller_set_direction(dev, config, data, SIGN(relative_steps));
			movement_steps_count =
				stepper_ramp_prepare_move(&data->ramp, abs(relative_steps));
		}

		/* Set target for position tracking */
		data->target_position = position;

		LOG_DBG("Movement will require %llu steps", movement_steps_count);

		if (movement_steps_count > 0) {
			/* Start/continue movement with calculated steps */
			stepper_motion_controller_handle_next_interval(dev, config, data);
		}
	}

	return 0;
}

/**
 * @brief Set the current position of the stepper motor (coordinate system reset)
 * @param dev The stepper device
 * @param position The new position value in micro steps
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_set_position(const struct device *dev, const int32_t position)
{
	struct stepper_motion_controller_data *data = get_data(dev);

	K_SPINLOCK(&data->lock) {
		LOG_DBG("Position reset from %d to %d", data->position, position);
		data->position = position;
	}

	return 0;
}

/**
 * @brief Get the current position of the stepper motor
 * @param dev The stepper device
 * @param position Pointer to store the current position in micro steps
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_get_position(const struct device *dev, int32_t *position)
{
	if (position == NULL) {
		return -EINVAL;
	}

	const struct stepper_motion_controller_data *data = get_data(dev);

	*position = data->position;
	return 0;
}

/**
 * @brief Run the stepper motor continuously in a specified direction
 *
 * The motor will run until explicitly stopped. Uses INT32_MAX/MIN as
 * target position to indicate continuous movement.
 *
 * @param dev The stepper device
 * @param direction The direction to run (POSITIVE or NEGATIVE)
 * @return 0 on success, negative error code on failure
 */
int stepper_motion_controller_run(const struct device *dev, enum stepper_direction direction)
{
	const struct stepper_motion_controller_config *config = get_config(dev);
	struct stepper_motion_controller_data *data = get_data(dev);

	const int32_t continuous_target = (direction == STEPPER_DIRECTION_POSITIVE)
						 ? CONTINUOUS_POSITIVE_POSITION
						 : CONTINUOUS_NEGATIVE_POSITION;

	K_SPINLOCK(&data->lock) {
		LOG_DBG("Continuous run started in direction %d", direction);
		
		/* Set continuous movement target */
		data->target_position = continuous_target;
		
		/* Start movement */
		stepper_motion_controller_set_direction(dev, config, data, direction);
		const uint64_t movement_steps_count = stepper_ramp_prepare_move(&data->ramp, INT32_MAX);
		
		if (movement_steps_count > 0) {
			stepper_motion_controller_handle_next_interval(dev, config, data);
		}
	}

	return 0;
}

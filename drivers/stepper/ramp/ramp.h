/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ramp.h
 * @brief stepper motor ramping algorithm definitions
 *
 * This header defines the data structures and APIs for stepper motor velocity
 * ramping, allowing acceleration and deceleration profiles (like trapezoidal).
 * It provides the foundation for implementing different ramping algorithms
 * that can be selected based on application requirements.
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_RAMP_H_
#define ZEPHYR_DRIVERS_STEPPER_RAMP_H_

#include <zephyr/drivers/stepper.h>
#include <zephyr/dt-bindings/stepper/ramp.h>
#include "../stepper_common.h"

/**
 * @brief Distance profile for stepper motor movement
 *
 * Defines the distances (in microsteps) for each phase of movement:
 * acceleration, constant speed, and deceleration.
 */
struct stepper_ramp_distance_profile {
	/** Distance covered during acceleration phase (µsteps) */
	uint32_t acceleration;
	/** Distance covered during constant speed phase (µsteps) */
	uint32_t const_speed;
	/** Distance covered during deceleration phase (µsteps) */
	uint32_t deceleration;
};

/**
 * @brief Ramp controller state machine states
 *
 * Defines the possible states of the ramp controller during operation.
 */
enum stepper_ramp_state {
	/** Motor is accelerating */
	STEPPER_RAMP_STATE_ACCELERATION,
	/** Motor is running at constant speed */
	STEPPER_RAMP_STATE_CONSTANT_SPEED,
	/** Motor is decelerating */
	STEPPER_RAMP_STATE_DECELERATION,
	/** Motor is prematurely decelerating before changing direction */
	STEPPER_RAMP_STATE_PRE_DECELERATION,
};

/**
 * @brief Runtime data for the ramp controller
 *
 * Contains the current state and position information needed by
 * the ramp controller during operation.
 */
struct stepper_ramp_runtime_data {
	/** Actual Ramp position in µsteps */
	uint32_t ramp_actual_position;
	/** Target position in µsteps */
	uint32_t ramp_target_position;
	/** Steps to be used while forced deceleration */
	uint32_t pre_deceleration_steps;
	/** Minimum step interval (in ns) threshold for stopping ramping */
	uint64_t ramp_stop_step_interval_threshold_in_ns;
	/** Flag indicating if the stepper direction has changed */
	bool is_stepper_dir_changed;
	/** Current state of the ramp controller */
	enum stepper_ramp_state current_ramp_state;
};

/**
 * @brief Configuration parameters for the ramp controller
 *
 * Contains const configuration values used by the ramp controller.
 */
struct stepper_ramp_config {
	const uint32_t pre_deceleration_steps;
};

/**
 * @brief Combined data structure for ramp controller
 *
 * Aggregates all data needed for ramp operation including runtime data,
 * distance profile, and ramp profile parameters.
 */
struct stepper_ramp_common {
	/** Runtime state data */
	struct stepper_ramp_runtime_data ramp_runtime_data;
	/** Distance profile information */
	struct stepper_ramp_distance_profile ramp_distance_profile;
	/** Ramp profile parameters (acceleration, max velocity) */
	struct stepper_ramp_profile ramp_profile;
};

/**
 * @brief Reset the ramp controller data
 *
 * @param config Pointer to ramp configuration
 * @param ramp_common Pointer to common ramp data
 * @param is_stepper_moving Flag indicating if stepper is currently moving
 * @param steps_to_move Number of steps to move
 */
typedef void (*stepper_ramp_reset_ramp_runtime_data_t)(const struct stepper_ramp_config *config,
						       struct stepper_ramp_common *ramp_common,
						       const bool is_stepper_moving,
						       const uint32_t steps_to_move);

/**
 * @brief Calculate the next step interval
 *
 * Calculates the time interval to the next step based on the current
 * state of the ramp controller.
 *
 * @param ramp_common Pointer to common ramp data
 * @param current_step_interval_in_ns Current step interval in nanoseconds
 * @param run_mode Current run mode (move to target or continuous run)
 * @return Next step interval in nanoseconds
 */
typedef uint64_t (*stepper_ramp_get_next_step_interval_t)(
	struct stepper_ramp_common *ramp_common, const uint64_t current_step_interval_in_ns,
	enum stepper_run_mode run_mode);

/**
 * @brief Recalculate the ramp profile
 *
 * Updates the ramp distance profile based on the total steps to move.
 *
 * @param ramp_common Pointer to common ramp data
 * @param total_steps_to_move Total steps to move
 */
typedef void (*stepper_ramp_recalculate_ramp_t)(struct stepper_ramp_common *ramp_common,
						uint32_t total_steps_to_move);

/**
 * @brief Calculate the starting step interval
 *
 * Determines the initial step interval based on acceleration parameters.
 *
 * @param acceleration Acceleration value in steps/s²
 * @return Initial step interval in nanoseconds
 */
typedef uint64_t (*stepper_calculate_start_interval_t)(const uint32_t acceleration);

/**
 * @brief API structure for ramp implementations
 *
 * Collection of function pointers that define the interface for
 * specific ramp algorithm implementations.
 */
struct stepper_ramp_api {
	stepper_ramp_reset_ramp_runtime_data_t reset_ramp_runtime_data;
	stepper_calculate_start_interval_t calculate_start_interval;
	stepper_ramp_recalculate_ramp_t recalculate_ramp;
	stepper_ramp_get_next_step_interval_t get_next_step_interval;
};

#define RAMP_DT_SPEC_GET_API(node_id)                                                              \
	(COND_CODE_1(DT_ENUM_HAS_VALUE(DT_DRV_INST(node_id),					   \
		ramp_type, STEPPER_RAMP_TYPE_TRAPEZOIDAL), (&trapezoidal_ramp_api), (NULL)))

#ifdef CONFIG_STEPPER_RAMP_TRAPEZOIDAL
extern const struct stepper_ramp_api trapezoidal_ramp_api;
#endif

#endif

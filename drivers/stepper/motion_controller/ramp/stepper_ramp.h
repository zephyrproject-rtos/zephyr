/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file stepper_ramp.h
 * @brief Stepper motor ramping algorithm definitions
 *
 * This header defines the data structures and APIs for stepper motor velocity
 * ramping, allowing acceleration and deceleration profiles (like trapezoidal).
 * It provides the foundation for implementing different ramping algorithms
 * that can be selected based on application requirements.
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_RAMP_H_
#define ZEPHYR_DRIVERS_STEPPER_RAMP_H_

#include <zephyr/drivers/stepper.h>
#include <stdint.h>

#include <zephyr/dt-bindings/stepper/stepper_ramp.h>

/**
 * Function pointer type for preparing stepper motor movement.
 *
 * This type defines a function that initializes and configures the movement parameters
 * for a stepper motor ramp. It calculates the necessary intervals and step counts for
 * acceleration, constant speed, and deceleration phases based on the provided specifications.
 *
 * @param ramp Pointer to the ramp data structure to be configured
 * @param step_count Number of steps to run.e
 *
 * @return The initial step interval in nanoseconds for the movement
 */
typedef uint64_t (*stepper_ramp_prepare_move_t)(struct stepper_ramp_base *ramp,
						uint32_t step_count);

typedef uint64_t (*stepper_ramp_prepare_stop_t)(struct stepper_ramp_base *ramp);

typedef uint64_t (*stepper_ramp_get_next_interval_t)(struct stepper_ramp_base *ramp);

struct stepper_ramp_api {
	/**
	 * Function pointer for preparing the movement profile of a stepper motor.
	 *
	 * This variable represents a function responsible for initializing and configuring
	 * the movement parameters for a stepper motor ramp, including acceleration,
	 * constant-speed, and deceleration phases.
	 *
	 * The function updates the internal ramp state and calculates the total number
	 * of steps required for the configured motion profile based on the given
	 * intervals, step count, and rates.
	 */
	stepper_ramp_prepare_move_t prepare_move;

	/**
	 * Function pointer for preparing the stop phase of a stepper motor motion profile.
	 *
	 * This variable represents a function tasked with determining the deceleration
	 * parameters required to bring a stepper motor to a controlled stop. It calculates
	 * the updated motion profile by adjusting the ramp state, current interval, and
	 * applying the specified deceleration rate to ensure smooth halting of the motor.
	 */
	stepper_ramp_prepare_stop_t prepare_stop;

	/**
	 * Function pointer for retrieving the interval until the next step in the stepper motor
	 * motion profile.
	 *
	 * This variable represents a function that calculates the time interval needed for the next
	 * step of the stepper motor based on the current state of the ramp data. It is used to
	 * manage the timing of motor steps during different phases of motion, such as acceleration,
	 * steady motion, and deceleration, ensuring precise control over the motor operation.
	 */
	stepper_ramp_get_next_interval_t get_next_interval;
};

inline uint64_t stepper_ramp_get_next_interval(struct stepper_ramp_base *ramp)
{
	return ramp->api->get_next_interval(ramp);
}

inline uint64_t stepper_ramp_prepare_move(struct stepper_ramp_base *ramp, uint32_t step_count)
{
	return ramp->api->prepare_move(ramp, step_count);
}

inline uint64_t stepper_ramp_prepare_stop(struct stepper_ramp_base *ramp)
{
	return ramp->api->prepare_stop(ramp);
}

#endif

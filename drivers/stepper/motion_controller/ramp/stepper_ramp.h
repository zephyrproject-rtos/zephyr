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

struct stepper_ramp_constant_data {
	uint64_t interval_ns;
	uint32_t steps_left;
};

/**
 * Represents the state of a motor control ramp used in stepper motor operation.
 *
 * This structure is designed to manage the different phases of motor control,
 * including acceleration, running at a steady speed, and deceleration. It holds
 * various counters and timing parameters required to implement smooth and efficient
 * motion profiles.
 */
struct stepper_ramp_trapezoidal_data {

	/**
	 * Represents the number of steps remaining during the pre-deceleration phase of a motor's
	 * ramp profile.
	 *
	 * This variable is used in the motor control logic to monitor the progress of the motor
	 * while speeding down. It helps ensure precise timing and smooth transitions as the motor
	 * decreases its speed to a lower velocity, contributing to accurate and efficient motion
	 * control.
	 */
	uint32_t pre_decel_steps_left;

	/**
	 * Represents the number of steps remaining during the acceleration phase of a motor's ramp
	 * profile.
	 *
	 * This variable is used in the motor control logic to monitor the progress of the motor
	 * while speeding up. It helps ensure precise timing and smooth transitions as the motor
	 * increases its speed, contributing to accurate and efficient motion control.
	 */
	uint32_t accel_steps_left;

	/**
	 * Represents the number of steps remaining to be executed in the current phase of motor
	 * control.
	 *
	 * This variable is a critical part in the motion control algorithm, used to monitor the
	 * progress of a motor in its ramp profile. It is decreased progressively as steps are
	 * executed, ensuring precise tracking of motor movement during acceleration,
	 * constant-speed, or deceleration phases. This contributes to achieving smooth, efficient,
	 * and accurate motor operations.
	 */
	uint32_t run_steps_left;

	/**
	 * Represents the number of steps remaining during the deceleration phase of a motor's ramp
	 * profile.
	 *
	 * This variable is used in motor control algorithms to track the progress of deceleration,
	 * ensuring smooth and accurate transitions as the motor slows down. It is progressively
	 * decreased as steps are executed, contributing to precise motion control and efficient
	 * deceleration management.
	 */
	uint32_t decel_steps_left;

	/**
	 * Represents the time interval, in nanoseconds, between consecutive motor steps
	 * during the constant speed phase of the ramp profile in motion control.
	 *
	 * This variable is crucial for defining the timing of motor operations when the motor
	 * runs at a steady speed. It determines the duration of each step to ensure consistent
	 * motion. Adjustments to this value influence motor performance, precision, and overall
	 * operational efficiency.
	 */
	uint64_t run_interval;

	/**
	 * Represents the time interval, in nanoseconds, for the very first step during
	 * the ramping profile of a motor's operation.
	 *
	 * This variable is critical in defining the initial timing of motor control
	 * when transitioning from a stationary state to motion. It ensures precise
	 * control at the onset of movement, establishing a foundation for later
	 * step intervals and enabling smooth acceleration. Proper handling of this
	 * interval contributes to accurate motor performance and operational consistency.
	 */
	uint64_t first_acceleration_interval;

	/**
	 * Represents the time interval, in nanoseconds, for the final step during
	 * the deceleration phase of a motor's ramp profile.
	 *
	 * This variable is essential in defining the precise timing for the last step
	 * as the motor completes its deceleration process. It ensures smooth and controlled
	 * transitions to a stop, contributing to overall motion accuracy and system stability.
	 */
	uint64_t last_deceleration_interval;

	/**
	 * Represents the fractional remainder of the time interval, in nanoseconds,
	 * accumulated between consecutive motor steps during a ramp profile operation.
	 *
	 * This variable is used in the motion control logic to account for precision timing
	 * discrepancies that may arise during the calculation of step intervals. It helps
	 * in maintaining accurate step timing by storing the remainder of division operations
	 * when calculating time intervals, thus ensuring smooth and consistent motor movement.
	 */
	uint64_t interval_calculation_rest;

	/**
	 * Indicates the current index in the acceleration or deceleration sequence of a motor's
	 * ramp profile.
	 *
	 * This variable is used to track the progression of calculation steps during both the
	 * acceleration and deceleration phases of the ramping logic. It plays a vital role in
	 * determining the adjustments needed for step intervals, ensuring accurate and smooth
	 * transitions in motor motion. Incremented or decremented depending on the operational
	 * phase, it is integral to precise motion control.
	 */
	uint32_t acceleration_idx;

	/**
	 * Represents the current time interval, in nanoseconds, for a motor step during
	 * its operational ramp profile, including acceleration, steady speed, or deceleration
	 * phases.
	 *
	 * This variable is used to define the duration between consecutive steps of a motor,
	 * ensuring precise control over timing. It plays a critical role in the motor control
	 * logic, enabling smooth transitions and consistent motion. Adjustments to this value
	 * directly impact the motor's performance, precision, and timing accuracy.
	 */
	uint64_t current_interval;
};

struct stepper_ramp {
	struct stepper_ramp_profile profile;
	union {
		struct stepper_ramp_constant_data constant;
		struct stepper_ramp_trapezoidal_data trapezoidal;
	} data;
};

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
typedef uint64_t (*stepper_ramp_prepare_move_t)(struct stepper_ramp *ramp, uint32_t step_count);

typedef uint64_t (*stepper_ramp_prepare_stop_t)(struct stepper_ramp *ramp);

typedef uint64_t (*stepper_ramp_get_next_interval_t)(struct stepper_ramp *ramp);

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

const struct stepper_ramp_api *stepper_ramp_get_api(const struct stepper_ramp *ramp);

uint64_t stepper_ramp_get_next_interval(struct stepper_ramp *ramp);

uint64_t stepper_ramp_prepare_move(struct stepper_ramp *ramp, const uint32_t step_count);

uint64_t stepper_ramp_prepare_stop(struct stepper_ramp *ramp);

#endif

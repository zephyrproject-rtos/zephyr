/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefaov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_TRAPEZOIDAL_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_TRAPEZOIDAL_H_

#include <zephyr/drivers/stepper.h>

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

/**
 * Defines the dynamic behavior of the ramp (acceleration and deceleration rates).
 * This can be part of a movement profile.
 */
struct stepper_ramp_trapezoidal_profile {
	/**
	 * Acceleration rate in steps/s/s to be used during the acceleration phase.
	 */
	uint32_t acceleration_rate;

	/**
	 * Interval in nanoseconds which should be reached after acceleration
	 * and used in the constant speed phase (target speed).
	 */
	uint64_t run_interval;

	/**
	 * Deceleration rate in steps/s/s to be used during the deceleration phase.
	 */
	uint32_t deceleration_rate;
};

struct stepper_ramp_trapezoidal {
	const struct stepper_ramp_api *const api;
	struct stepper_ramp_trapezoidal_data *const data;
	struct stepper_ramp_trapezoidal_profile *const profile;
};

#ifdef CONFIG_STEPPER_RAMP_TRAPEZOIDAL
extern const struct stepper_ramp_api stepper_ramp_trapezoidal_api;

#define STEPPER_RAMP_TRAPEZOIDAL_DEFINE(name, acceleration_rate_p, run_interval_p,                 \
					deceleration_rate_p)                                       \
	struct stepper_ramp_trapezoidal_profile name##_profile = {                                 \
		.acceleration_rate = acceleration_rate_p,                                          \
		.run_interval = run_interval_p,                                                    \
		.deceleration_rate = deceleration_rate_p,                                          \
	};                                                                                         \
	struct stepper_ramp_trapezoidal_data name##_data = {                                       \
		.acceleration_idx = 0,                                                             \
	};                                                                                         \
	struct stepper_ramp_trapezoidal name##_ramp = {                                            \
		.api = &stepper_ramp_trapezoidal_api,                                              \
		.data = &name##_data,                                                              \
		.profile = &name##_profile,                                                        \
	};                                                                                         \
	struct stepper_ramp_base *name = (struct stepper_ramp_base *)&name##_ramp;

#endif /* CONFIG_STEPPER_RAMP_TRAPEZOIDAL */

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_RAMP_TRAPEZOIDAL_H_ */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stepper_ramp.h"

#include <limits.h>
#include <sys/errno.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stepper_ramp_constant, CONFIG_STEPPER_LOG_LEVEL);

/**
 * Prepares a constant velocity movement with the given profile and step count.
 * This implementation ignores acceleration and deceleration values and uses
 * only the run_interval value from the profile.
 * 
 * For continuous movement, pass INT32_MAX as step_count to run indefinitely
 * until explicitly stopped.
 *
 * @param ramp Pointer to the ramp data structure
 * @param step_count Number of steps to move, or INT32_MAX for continuous movement
 *
 * @return Total number of steps in the movement or negative error code
 */
static uint64_t prepare_move(struct stepper_ramp *ramp, const uint32_t step_count)
{
	const bool continuous_movement = (step_count == INT32_MAX);
	
	LOG_DBG("Prepare constant velocity movement by %u steps (continuous: %s)", 
		step_count, continuous_movement ? "true" : "false");

	struct stepper_ramp_constant_data *data = &ramp->data.constant;
	const struct stepper_ramp_square_profile *profile = &ramp->profile.square;

	data->steps_left = step_count;
	data->interval_ns = profile->interval_ns;

	return step_count;
}

/**
 * Prepares a stop without deceleration - immediately stops motion.
 *
 * @param ramp Pointer to the ramp data structure
 *
 * @return Always returns 0 as there are no deceleration steps
 */
static uint64_t prepare_stop(struct stepper_ramp *ramp)
{
	LOG_DBG("Prepare immediate stop");

	struct stepper_ramp_constant_data *data = &ramp->data.constant;

	data->steps_left = 0;

	return 0;
}

/**
 * Gets the next step interval for the constant velocity profile.
 * This will always return the run_interval until the movement is complete.
 * For continuous movement (INT32_MAX steps), it runs indefinitely without
 * decrementing the step counter.
 *
 * @param ramp Pointer to the ramp data structure
 * @return The current interval value or 0 if the movement is finished
 */
static uint64_t get_next_interval(struct stepper_ramp *ramp)
{
	struct stepper_ramp_constant_data *data = &ramp->data.constant;

	if (data->steps_left > 0) {
		/* For continuous movement (INT32_MAX), don't decrement to avoid underflow */
		if (data->steps_left != INT32_MAX) {
			data->steps_left--;
		}
		return data->interval_ns;
	}

	/* movement finished */
	return 0;
}

/* API interface for the constant velocity ramp generator */
const struct stepper_ramp_api stepper_ramp_constant_api = {
	.prepare_move = prepare_move,
	.prepare_stop = prepare_stop,
	.get_next_interval = get_next_interval,
};

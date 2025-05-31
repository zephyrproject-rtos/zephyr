/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stepper_ramp.h"
#include <zephyr/drivers/stepper/stepper_ramp_constant.h>

#include <sys/errno.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stepper_ramp_constant, CONFIG_STEPPER_LOG_LEVEL);

/**
 * Prepares a constant velocity movement with the given profile and step count.
 * This implementation ignores acceleration and deceleration values and uses
 * only the run_interval value from the profile.
 *
 * @param ramp Pointer to the ramp data structure
 * @param step_count Number of steps to move
 *
 * @return Total number of steps in the movement or negative error code
 */
static uint64_t prepare_move(struct stepper_ramp_base *ramp, const uint32_t step_count)
{
	LOG_DBG("Prepare constant velocity movement by %u steps", step_count);

	const struct stepper_ramp_constant *self = (struct stepper_ramp_constant *)ramp;
	struct stepper_ramp_constant_data *data = self->data;
	const struct stepper_ramp_constant_profile *profile = self->profile;

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
static uint64_t prepare_stop(struct stepper_ramp_base *ramp)
{
	LOG_DBG("Prepare immediate stop");

	const struct stepper_ramp_constant *ramp_const = (struct stepper_ramp_constant *)ramp;
	struct stepper_ramp_constant_data *data = ramp_const->data;

	data->steps_left = 0;

	return 0;
}

/**
 * Gets the next step interval for the constant velocity profile.
 * This will always return the run_interval until the movement is complete.
 *
 * @param ramp Pointer to the ramp data structure
 * @return The current interval value or 0 if movement is finished
 */
static uint64_t get_next_interval(struct stepper_ramp_base *ramp)
{
	const struct stepper_ramp_constant *self = (struct stepper_ramp_constant *)ramp;
	struct stepper_ramp_constant_data *data = self->data;

	if (data->steps_left > 0) {
		data->steps_left--;
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

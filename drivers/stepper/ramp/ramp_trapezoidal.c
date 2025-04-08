/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "ramp.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ramp_control, CONFIG_STEPPER_LOG_LEVEL);

static bool is_dir_changed_while_in_motion(const bool is_stepper_moving,
					   const bool is_stepper_dir_changed)
{
	if (is_stepper_moving && is_stepper_dir_changed) {
		LOG_DBG("Direction changed while in motion");
		return true;
	}
	return false;
}

static uint64_t calc_approx_step_interval(const uint64_t current_step_interval,
					  const uint32_t ramp_position,
					  enum stepper_ramp_state state)
{
	uint64_t new_step_interval = current_step_interval;

	switch (state) {
	case STEPPER_RAMP_STATE_ACCELERATION:
		new_step_interval = current_step_interval -
				    (2 * current_step_interval) / (4 * ramp_position + 1);
		break;
	case STEPPER_RAMP_STATE_PRE_DECELERATION:
	case STEPPER_RAMP_STATE_DECELERATION:
		new_step_interval = current_step_interval +
				    (2 * current_step_interval) / (4 * ramp_position + 1);
		break;
	default:
		break;
	}
	return new_step_interval;
}

/**
 * @brief Simple recursive implementation of an integer square root
 *
 * copied from samples/kernel/metairq_dispatch/src/main.c 0b90fd5
 */
static uint32_t isqrt(uint64_t n)
{
	if (n <= 1) {
		return (uint32_t)n;
	}

	uint64_t x = n;
	uint64_t y = (x + 1) / 2;

	while (y < x) {
		x = y;
		y = (x + n / x) / 2;
	}
	return (uint32_t)x;
}

static uint64_t trapezpoidal_calculate_start_interval(uint32_t acceleration)
{
	if (acceleration == 0) {
		LOG_ERR("Error: Acceleration cannot be zero");
		return UINT64_MAX;
	}

	/* the value of (2 * factor * factor) may not overflow uint64_t but at the same time be as
	 * large as possible to ensure maximal possible precision of isqrt */
	const uint64_t factor = 3037000499ULL;

	/* Calculate the start velocity based on the acceleration
	 *
	 * Using the formula: t = f * sqrt(2 * d / a)
	 * where f = counter frequency, d = 1 step, a = acceleration
	 *
	 * This value will be used in approximation as described in AVR446 section 2.3.1
	 * The approximation introduces an error which has to be corrected by multiplying first
	 * interval by factor of 0.676 The resulting formula is:
	 *
	 * start_interval = f * sqrt(2 / acceleration) * 0.676
	 *
	 * Since division of integer 2 by acceleration is problematic without usage of floating
	 * points,the formula is rewritten as:
	 *
	 * start_interval = f * sqrt(2 * factor * factor / acceleration) / factor */
	const uint64_t step_interval_in_ns = NSEC_PER_SEC * 676ULL / 1000ULL *
					     isqrt(2ULL * factor * factor / acceleration) / factor;
	LOG_DBG("Start Interval in ns: %llu", step_interval_in_ns);
	return step_interval_in_ns;
}

static void trapezoidal_recalculate_ramp(struct stepper_ramp_common *ramp_common,
					 const uint32_t total_steps_to_move)
{
	struct stepper_ramp_profile *ramp_profile = &ramp_common->ramp_profile;
	struct stepper_ramp_distance_profile *distance_profile =
		&ramp_common->ramp_distance_profile;
	distance_profile->acceleration = (ramp_profile->max_velocity * ramp_profile->max_velocity) /
					 (ramp_profile->acceleration * 2);

	distance_profile->deceleration = (ramp_profile->max_velocity * ramp_profile->max_velocity) /
					 (ramp_profile->deceleration * 2);

	if ((distance_profile->acceleration + distance_profile->deceleration) >
	    total_steps_to_move) {
		LOG_DBG("Total distance to move is less than the sum of acceleration and "
			"deceleration distances");
		distance_profile->const_speed = 0;
		distance_profile->deceleration =
			(total_steps_to_move * ramp_profile->acceleration) /
			(ramp_profile->deceleration + ramp_profile->acceleration);
		distance_profile->acceleration =
			total_steps_to_move - distance_profile->deceleration;
		LOG_DBG("Recalculating Distance Profile: acceleration=%d "
			"const_speed=%d "
			"deceleration=%d for total_steps=%d",
			distance_profile->acceleration, distance_profile->const_speed,
			distance_profile->deceleration, total_steps_to_move);
	} else {
		distance_profile->const_speed = total_steps_to_move -
						distance_profile->acceleration -
						distance_profile->deceleration;
		LOG_DBG("Distance Profile: acceleration=%d const_speed=%d "
			"deceleration=%d",
			distance_profile->acceleration, distance_profile->const_speed,
			distance_profile->deceleration);
	}
}

static void increment_ramp_position(struct stepper_ramp_runtime_data *ramp_data)
{
	if (ramp_data->ramp_actual_position < ramp_data->ramp_target_position) {
		ramp_data->ramp_actual_position++;
	}
}

static uint32_t get_remaining_ramp_steps(const struct stepper_ramp_runtime_data *ramp_data)
{
	return ramp_data->ramp_target_position - ramp_data->ramp_actual_position;
}

static uint64_t trapezoidal_get_next_step_interval(struct stepper_ramp_common *ramp_common,
						   const uint64_t current_step_interval_in_ns,
						   enum stepper_run_mode run_mode)
{
	struct stepper_ramp_runtime_data *ramp_data = &ramp_common->ramp_runtime_data;
	struct stepper_ramp_profile *ramp_profile = &ramp_common->ramp_profile;
	struct stepper_ramp_distance_profile *distance_profile =
		&ramp_common->ramp_distance_profile;
	const uint32_t max_velocity = ramp_profile->max_velocity;
	uint64_t new_step_interval_in_ns = current_step_interval_in_ns;

	if (current_step_interval_in_ns < NSEC_PER_SEC / max_velocity &&
	    ramp_data->current_ramp_state != STEPPER_RAMP_STATE_DECELERATION) {
		LOG_DBG("Moving to constant speed");
		ramp_data->current_ramp_state = STEPPER_RAMP_STATE_CONSTANT_SPEED;
	}

	if (get_remaining_ramp_steps(ramp_data) <= distance_profile->deceleration &&
	    ramp_data->current_ramp_state != STEPPER_RAMP_STATE_DECELERATION &&
	    run_mode == STEPPER_RUN_MODE_POSITION) {
		LOG_DBG("Moving to deceleration");
		ramp_data->current_ramp_state = STEPPER_RAMP_STATE_DECELERATION;
	}

	switch (ramp_data->current_ramp_state) {
	case STEPPER_RAMP_STATE_ACCELERATION:
		increment_ramp_position(ramp_data);
		new_step_interval_in_ns = calc_approx_step_interval(current_step_interval_in_ns,
								    ramp_data->ramp_actual_position,
								    ramp_data->current_ramp_state);
		break;

	case STEPPER_RAMP_STATE_CONSTANT_SPEED:
		increment_ramp_position(ramp_data);
		new_step_interval_in_ns = NSEC_PER_SEC / max_velocity;
		break;

	/**
	 * Guard to decelerate till the ramp stop step interval threshold is reached.
	 */
	case STEPPER_RAMP_STATE_DECELERATION:
		increment_ramp_position(ramp_data);
		if ((current_step_interval_in_ns <=
		     ramp_data->ramp_stop_step_interval_threshold_in_ns)) {
			new_step_interval_in_ns = calc_approx_step_interval(
				current_step_interval_in_ns, distance_profile->deceleration--,
				ramp_data->current_ramp_state);
		}
		break;

	/**
	 * In case of forced deceleration, the step interval is calculated based on the
	 * number of steps left to be moved. The step interval is increased by a factor of
	 * 2/(4*n+1) where n is the number of steps left to be moved.
	 */
	case STEPPER_RAMP_STATE_PRE_DECELERATION:
		if ((current_step_interval_in_ns >
		     ramp_data->ramp_stop_step_interval_threshold_in_ns)) {
			uint64_t start_velocity =
				trapezpoidal_calculate_start_interval(ramp_profile->acceleration);
			LOG_DBG("Step Interval in ns: %llu", start_velocity);
			LOG_DBG("Forced deceleration completed");
			ramp_data->current_ramp_state = STEPPER_RAMP_STATE_ACCELERATION;
			trapezoidal_recalculate_ramp(ramp_common, ramp_data->ramp_target_position);
			return start_velocity;
		}

		if (ramp_data->pre_deceleration_steps > 0) {
			ramp_data->pre_deceleration_steps--;
		}

		/* Increasing ramp target position as this amount of steps would have to be
		 * traversed back
		 */
		ramp_data->ramp_target_position++;

		new_step_interval_in_ns = calc_approx_step_interval(
			current_step_interval_in_ns, ramp_data->pre_deceleration_steps,
			ramp_data->current_ramp_state);
		break;
	default:
		new_step_interval_in_ns = current_step_interval_in_ns;
		break;
	}
	return new_step_interval_in_ns;
}

static void trapezoidal_reset_ramp_runtime_data(const struct stepper_ramp_config *config,
						struct stepper_ramp_common *ramp_common_data,
						const bool is_stepper_dir_changed,
						const bool is_stepper_moving,
						const uint32_t steps_to_move)
{
	struct stepper_ramp_runtime_data *data = &ramp_common_data->ramp_runtime_data;
	struct stepper_ramp_profile *ramp_profile = &ramp_common_data->ramp_profile;

	if (is_dir_changed_while_in_motion(is_stepper_moving, is_stepper_dir_changed)) {
		data->current_ramp_state = STEPPER_RAMP_STATE_PRE_DECELERATION;
	} else if (!is_stepper_moving) {
		data->current_ramp_state = STEPPER_RAMP_STATE_NOT_MOVING;
	}
	LOG_DBG("Resetting ramp data for ramp state %d", data->current_ramp_state);

	switch (data->current_ramp_state) {
	case STEPPER_RAMP_STATE_PRE_DECELERATION:
		data->pre_deceleration_steps = config->pre_deceleration_steps;
		data->ramp_actual_position = 0;
		data->ramp_target_position = steps_to_move;
		break;

	case STEPPER_RAMP_STATE_NOT_MOVING:
		ramp_common_data->ramp_runtime_data.ramp_stop_step_interval_threshold_in_ns =
			trapezpoidal_calculate_start_interval(ramp_profile->acceleration);
		data->current_ramp_state = STEPPER_RAMP_STATE_ACCELERATION;
		data->ramp_actual_position = 0;
		data->ramp_target_position = steps_to_move;
		break;

	case STEPPER_RAMP_STATE_DECELERATION:
		data->current_ramp_state = STEPPER_RAMP_STATE_ACCELERATION;
		data->ramp_actual_position = get_remaining_ramp_steps(data);
		data->ramp_target_position = data->ramp_actual_position + steps_to_move;
		break;
	case STEPPER_RAMP_STATE_ACCELERATION:
	case STEPPER_RAMP_STATE_CONSTANT_SPEED:
		data->ramp_target_position = data->ramp_actual_position + steps_to_move;
		break;
	default:
		break;
	}
}

const struct stepper_ramp_api trapezoidal_ramp_api = {
	.reset_ramp_runtime_data = trapezoidal_reset_ramp_runtime_data,
	.calculate_start_interval = trapezpoidal_calculate_start_interval,
	.recalculate_ramp = trapezoidal_recalculate_ramp,
	.get_next_step_interval = trapezoidal_get_next_step_interval,
};

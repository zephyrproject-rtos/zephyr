/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <zephyr/ztest.h>

#include "../../../drivers/stepper/ramp/ramp.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ramp, CONFIG_STEPPER_LOG_LEVEL);

#define RAMP_PROFILE(accel, velocity, decel)                                                       \
	{                                                                                          \
		.acceleration = accel,                                                             \
		.max_velocity = velocity,                                                          \
		.deceleration = decel,                                                             \
	}

#define DISTANCE_PROFILE(accel, const_velocity, decel)                                             \
	{                                                                                          \
		.acceleration = accel,                                                             \
		.const_speed = const_velocity,                                                     \
		.deceleration = decel,                                                             \
	}

#define RAMP_RUNTIME_DATA(actual_pos, target_pos, pre_decel_steps, threshold, state)               \
	(struct stepper_ramp_runtime_data)                                                         \
	{                                                                                          \
		.ramp_actual_position = actual_pos, .ramp_target_position = target_pos,            \
		.pre_deceleration_steps = pre_decel_steps,                                         \
		.ramp_stop_step_interval_threshold_in_ns = threshold, .current_ramp_state = state, \
	}

#define RAMP_TEST_PARAMS(accel, velocity, decel, pre_decel, steps)                                 \
	(struct test_params)                                                                       \
	{                                                                                          \
		.acceleration = accel, .max_velocity = velocity, .deceleration = decel,            \
		.pre_deceleration_steps = pre_decel, .steps_to_move = steps,                       \
	}

struct test_params {
	uint16_t acceleration;
	uint32_t max_velocity;
	uint16_t deceleration;
	uint32_t pre_deceleration_steps;
	uint32_t steps_to_move;
};

struct reset_ramp_data_expectation {
	struct stepper_ramp_distance_profile distance_profile;
};

struct get_next_step_interval_expectation {
	uint64_t *intervals;
	uint32_t count;
};

static void test_reset_ramp_data(const struct test_params params,
				 const struct reset_ramp_data_expectation expectation)
{
	struct stepper_ramp_common common = {
		.ramp_profile =
			RAMP_PROFILE(params.acceleration, params.max_velocity, params.deceleration),
		.ramp_distance_profile = DISTANCE_PROFILE(0, 0, 0),
	};

	struct stepper_ramp_config config = {
		.pre_deceleration_steps = params.pre_deceleration_steps,
	};

	trapezoidal_ramp_api.reset_ramp_runtime_data(&config, &common, params.steps_to_move);

	trapezoidal_ramp_api.recalculate_ramp(&common, params.steps_to_move);

	zexpect_equal(common.ramp_distance_profile.acceleration +
			      common.ramp_distance_profile.const_speed +
			      common.ramp_distance_profile.deceleration,
		      params.steps_to_move, "Expected total steps %d but got %d",
		      params.steps_to_move,
		      common.ramp_distance_profile.acceleration +
			      common.ramp_distance_profile.const_speed +
			      common.ramp_distance_profile.deceleration);

	zexpect_equal(common.ramp_distance_profile.acceleration,
		      expectation.distance_profile.acceleration,
		      "Expected acceleration %d but got %d",
		      expectation.distance_profile.acceleration,
		      common.ramp_distance_profile.acceleration);

	zexpect_equal(
		common.ramp_distance_profile.const_speed, expectation.distance_profile.const_speed,
		"Expected const_speed %d but got %d", expectation.distance_profile.const_speed,
		common.ramp_distance_profile.const_speed);

	zexpect_equal(common.ramp_distance_profile.deceleration,
		      expectation.distance_profile.deceleration,
		      "Expected deceleration %d but got %d",
		      expectation.distance_profile.deceleration,
		      common.ramp_distance_profile.deceleration);
}

ZTEST_SUITE(ramp, NULL, NULL, NULL, NULL, NULL);

ZTEST(ramp, test_ramp_distance_profile)
{
	/** motion of 0 steps should not result in steps in any ramp phase */
	LOG_DBG("Test zero steps");
	test_reset_ramp_data(RAMP_TEST_PARAMS(1000, 10000, 1000, 0, 0),
			     (struct reset_ramp_data_expectation){
				     .distance_profile = DISTANCE_PROFILE(0, 0, 0),
			     });

	/** motion of 1 step should result in acceleration of 1 step if the requested velocity is
	 * higher than the start velocity of the ramp.
	 */
	LOG_DBG("Test 1 fast step");
	test_reset_ramp_data(RAMP_TEST_PARAMS(1000, 10000, 1000, 0, 1),
			     (struct reset_ramp_data_expectation){
				     .distance_profile = DISTANCE_PROFILE(1, 0, 0),
			     });

	/** motion of 1 step should result in a single constant speed step if the requested
	 * velocityis lower than the start velocity of the ramp.
	 */
	LOG_DBG("Test 1 slow step");
	test_reset_ramp_data(RAMP_TEST_PARAMS(1000, 1, 1000, 0, 1),
			     (struct reset_ramp_data_expectation){
				     .distance_profile = DISTANCE_PROFILE(0, 1, 0),
			     });

	LOG_DBG("Test 2 steps");
	test_reset_ramp_data(RAMP_TEST_PARAMS(1000, 10000, 1000, 0, 2),
			     (struct reset_ramp_data_expectation){
				     .distance_profile = DISTANCE_PROFILE(1, 0, 1),
			     });

	LOG_DBG("Test 3 steps");
	test_reset_ramp_data(RAMP_TEST_PARAMS(1000, 10000, 1000, 0, 3),
			     (struct reset_ramp_data_expectation){
				     .distance_profile = DISTANCE_PROFILE(2, 0, 1),
			     });

	LOG_DBG("Test 1000 steps");
	test_reset_ramp_data(RAMP_TEST_PARAMS(1000, 10000, 1000, 0, 1000),
			     (struct reset_ramp_data_expectation){
				     .distance_profile = DISTANCE_PROFILE(500, 0, 500),
			     });

	LOG_DBG("Test 1001 steps");
	test_reset_ramp_data(RAMP_TEST_PARAMS(1000, 10000, 1000, 0, 1001),
			     (struct reset_ramp_data_expectation){
				     .distance_profile = DISTANCE_PROFILE(501, 0, 500),
			     });

	LOG_DBG("Test 110000 steps");
	test_reset_ramp_data(RAMP_TEST_PARAMS(1000, 10000, 1000, 0, 110000),
			     (struct reset_ramp_data_expectation){
				     .distance_profile = DISTANCE_PROFILE(50000, 10000, 50000),
			     });
}

ZTEST(ramp, test_first_interval)
{
	uint32_t test_accelerations[] = {1, 100, 1000, UINT32_MAX};

	for (size_t i = 0; i < ARRAY_SIZE(test_accelerations); i++) {
		LOG_DBG("Test acceleration %u steps/s/s", test_accelerations[i]);
		const uint64_t start_interval =
			trapezoidal_ramp_api.calculate_start_interval(test_accelerations[i]);
		LOG_DBG("Start interval in ns: %llu", start_interval);

		/** see AVR446 section 2.3.1, exact calculations of the inter-step delay */
		const double ideal_start_interval =
			NSEC_PER_SEC * sqrt(2.0 / test_accelerations[i]) * 0.676;
		LOG_DBG("Ideal start interval in ns: %f", ideal_start_interval);

		const double ratio = fabs((double)start_interval / ideal_start_interval);

		zassert_within(ratio, 1.0, 0.01);
	}

	LOG_DBG("Test acceleration %u steps/s/s", 0);
	const uint64_t invalid_interval = trapezoidal_ramp_api.calculate_start_interval(0);

	zassert_equal(invalid_interval, UINT64_MAX);
}

ZTEST(ramp, test_get_next_step_interval)
{
	struct stepper_ramp_common common = {
		.ramp_profile = RAMP_PROFILE(1000, 10000, 1000),
		.ramp_distance_profile = DISTANCE_PROFILE(0, 0, 0),
		.ramp_runtime_data = RAMP_RUNTIME_DATA(0, 0, 0, 0, STEPPER_RAMP_STATE_NOT_MOVING),
	};

	const struct stepper_ramp_config config = {
		.pre_deceleration_steps = 0,
	};

	trapezoidal_ramp_api.reset_ramp_runtime_data(&config, &common, 1000);
	trapezoidal_ramp_api.recalculate_ramp(&common, 1000);

	uint64_t intervals[500] = {0};

	intervals[0] = trapezoidal_ramp_api.calculate_start_interval(1000);

	for (uint32_t i = 1; i < 500; i++) {
		intervals[i] = trapezoidal_ramp_api.get_next_step_interval(
			&common, intervals[i - 1], STEPPER_RUN_MODE_POSITION);
	}

	for (uint32_t i = 1; i < 500; i++) {
		LOG_DBG("Compare %llu vs %llu", intervals[i - 1], intervals[i]);
		zexpect(intervals[i - 1] > intervals[i],
			"Acceleration intervals have to decrease over time");
	}

	zexpect(NSEC_PER_SEC / 1000 < intervals[499],
		"Last ramp interval has to be smaller than the requested velocity");
}

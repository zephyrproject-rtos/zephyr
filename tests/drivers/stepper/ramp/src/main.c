/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/drivers/stepper/stepper_ramp.h>
#include "../../../drivers/stepper/ramp/ramp.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ramp, CONFIG_STEPPER_LOG_LEVEL);

struct test_params
{
	uint32_t acceleration;
	uint32_t max_velocity;
	uint32_t deceleration;
	uint32_t pre_deceleration_steps;
	uint32_t steps_to_move;
};

struct reset_ramp_data_expectation
{
	struct stepper_ramp_distance_profile distance_profile;
};

struct get_next_step_interval_expectation
{
	uint64_t *intervals;
	uint32_t count;
};

static void test_reset_ramp_data(struct test_params params,
                                 struct reset_ramp_data_expectation expectation)
{
	struct stepper_ramp_common common = {
		.ramp_profile =
		{
			.acceleration = params.acceleration,
			.max_velocity = params.max_velocity,
			.deceleration = params.deceleration,
		},
		.ramp_distance_profile =
		{
			.acceleration = 0,
			.const_speed = 0,
			.deceleration = 0,
		},
	};

	struct stepper_ramp_config config = {
		.pre_deceleration_steps = params.pre_deceleration_steps,
	};

	trapezoidal_ramp_api.reset_ramp_data(
		&config, &common, false,
		false, params.steps_to_move);

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
		"Expected const_speed %d but got %d",
		expectation.distance_profile.const_speed,
		common.ramp_distance_profile.const_speed);

	zexpect_equal(common.ramp_distance_profile.deceleration,
	              expectation.distance_profile.deceleration,
	              "Expected deceleration %d but got %d",
	              expectation.distance_profile.deceleration,
	              common.ramp_distance_profile.deceleration);
}

static void test_get_next_step_interval(struct test_params params,
                                        struct get_next_step_interval_expectation expectation)
{
	struct stepper_ramp_common common = {
		.ramp_profile =
		{
			.acceleration = params.acceleration,
			.max_velocity = params.max_velocity,
			.deceleration = params.deceleration,
		},
		.ramp_distance_profile =
		{
			.acceleration = 0,
			.const_speed = 0,
			.deceleration = 0,
		},
		.ramp_data =
		{
			.ramp_actual_position = 0,
			.ramp_target_position = params.steps_to_move,
			.forced_deceleration_steps = 0,
			.ramp_stop_step_interval_threshold_in_ns = 0,
			.current_ramp_state = STEPPER_RAMP_STATE_NOT_MOVING,
		},
	};

	struct stepper_ramp_config config = {
		.pre_deceleration_steps = params.pre_deceleration_steps,
	};

	uint64_t step_interval_in_ns = 0;
	trapezoidal_ramp_api.reset_ramp_data(&config, &common, false, false,
	                                     params.steps_to_move);

	trapezoidal_ramp_api.recalculate_ramp(&common, params.steps_to_move);

	step_interval_in_ns = trapezoidal_ramp_api.calculate_start_interval(params.acceleration);
	LOG_INF("Step interval in ns: %llu", step_interval_in_ns);

	for (uint32_t i = 0; i < expectation.count; i++) {
		step_interval_in_ns =
			trapezoidal_ramp_api.get_next_step_interval(&common, step_interval_in_ns, STEPPER_RUN_MODE_POSITION);

		zexpect_equal(step_interval_in_ns, expectation.intervals[i],
		              "Expected step interval %llu for step %u but got %llu",
		              expectation.intervals[i], i, step_interval_in_ns);
	}
}

ZTEST_SUITE(ramp, NULL, NULL, NULL, NULL, NULL);

ZTEST(ramp, test_ramp_distance_profile)
{
	LOG_INF("Test zero steps");
	test_reset_ramp_data(
		(struct test_params){
			.acceleration = 1000,
			.max_velocity = 10000,
			.deceleration = 1000,
			.pre_deceleration_steps = 0,
			.steps_to_move = 0,
		},
		(struct reset_ramp_data_expectation){
			.distance_profile =
			{
				.acceleration = 0,
				.const_speed = 0,
				.deceleration = 0,
			},
		});

	LOG_INF("Test 1 step");
	test_reset_ramp_data(
		(struct test_params){
			.acceleration = 1000,
			.max_velocity = 10000,
			.deceleration = 1000,
			.pre_deceleration_steps = 0,
			.steps_to_move = 1,
		},
		(struct reset_ramp_data_expectation){
			.distance_profile =
			{
				.acceleration = 1,
				.const_speed = 0,
				.deceleration = 0,
			},
		});

	LOG_INF("Test 2 steps");
	test_reset_ramp_data(
		(struct test_params){
			.acceleration = 1000,
			.max_velocity = 10000,
			.deceleration = 1000,
			.pre_deceleration_steps = 0,
			.steps_to_move = 2,
		},
		(struct reset_ramp_data_expectation){
			.distance_profile =
			{
				.acceleration = 1,
				.const_speed = 0,
				.deceleration = 1,
			},
		});

	LOG_INF("Test 3 steps");
	test_reset_ramp_data(
		(struct test_params){
			.acceleration = 1000,
			.max_velocity = 10000,
			.deceleration = 1000,
			.pre_deceleration_steps = 0,
			.steps_to_move = 3,
		},
		(struct reset_ramp_data_expectation){
			.distance_profile =
			{
				.acceleration = 2,
				.const_speed = 0,
				.deceleration = 1,
			},
		});

	LOG_INF("Test 100000 steps");
	test_reset_ramp_data(
		(struct test_params){
			.acceleration = 1000,
			.max_velocity = 10000,
			.deceleration = 1000,
			.pre_deceleration_steps = 0,
			.steps_to_move = 100000,
		},
		(struct reset_ramp_data_expectation){
			.distance_profile =
			{
				.acceleration = 50000,
				.const_speed = 0,
				.deceleration = 50000,
			},
		});

	LOG_INF("Test 100001 steps");
	test_reset_ramp_data(
		(struct test_params){
			.acceleration = 1000,
			.max_velocity = 10000,
			.deceleration = 1000,
			.pre_deceleration_steps = 0,
			.steps_to_move = 100001,
		},
		(struct reset_ramp_data_expectation){
			.distance_profile =
			{
				.acceleration = 50000,
				.const_speed = 1,
				.deceleration = 50000,
			},
		});

	LOG_INF("Test 200000 steps");
	test_reset_ramp_data(
		(struct test_params){
			.acceleration = 1000,
			.max_velocity = 10000,
			.deceleration = 1000,
			.pre_deceleration_steps = 0,
			.steps_to_move = 200000,
		},
		(struct reset_ramp_data_expectation){
			.distance_profile =
			{
				.acceleration = 50000,
				.const_speed = 100000,
				.deceleration = 50000,
			},
		});
}

ZTEST(ramp, test_get_next_step_interval)
{
	uint64_t expected_intervals[100] = {0};
	test_get_next_step_interval(
		(struct test_params){
			.acceleration = 100,
			.max_velocity = 100,
			.deceleration = 100,
			.pre_deceleration_steps = 0,
			.steps_to_move = 100,
		},
		(struct get_next_step_interval_expectation){
			.intervals = expected_intervals,
			.count = 100,
		});
}

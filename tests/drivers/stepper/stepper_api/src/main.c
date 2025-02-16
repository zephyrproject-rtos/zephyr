/*
 * Copyright 2025 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest_assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/stepper.h>

struct stepper_api_fixture {
	const struct device *dev;
	stepper_event_callback_t callback;
	uint32_t base_ms_resolution;
	uint32_t test_ms_resolution;
	uint32_t incorrect_ms_resoltion;
	uint32_t test_steps;
	uint64_t test_interval;
};

struct k_poll_signal stepper_signal;
struct k_poll_event stepper_event;

static void stepper_api_print_event_callback(const struct device *dev, enum stepper_event event,
					     void *dummy)
{
	switch (event) {
	case STEPPER_EVENT_STEPS_COMPLETED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_STEPS_COMPLETED);
		break;
	case STEPPER_EVENT_LEFT_END_STOP_DETECTED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_LEFT_END_STOP_DETECTED);
		break;
	case STEPPER_EVENT_RIGHT_END_STOP_DETECTED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_RIGHT_END_STOP_DETECTED);
		break;
	case STEPPER_EVENT_STALL_DETECTED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_STALL_DETECTED);
		break;
	default:
		break;
	}
}

static void *stepper_api_setup(void)
{
	static struct stepper_api_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(stepper_motor)),
		.callback = stepper_api_print_event_callback,
		.base_ms_resolution = CONFIG_BASE_MS_RESOLUTION,
		.test_ms_resolution = CONFIG_TEST_MS_RESOLUTION,
		.incorrect_ms_resoltion = CONFIG_INCORRECT_MS_RESOLUTION,
		.test_steps = CONFIG_SPEED_STEPS,
		.test_interval = NSEC_PER_SEC / CONFIG_SPEED_STEPS,
	};

	k_poll_signal_init(&stepper_signal);
	k_poll_event_init(&stepper_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &stepper_signal);

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void stepper_api_before(void *f)
{
	struct stepper_api_fixture *fixture = f;
	(void)stepper_set_reference_position(fixture->dev, 0);
	(void)stepper_set_micro_step_res(fixture->dev, fixture->base_ms_resolution);
	k_poll_signal_reset(&stepper_signal);
}

static void stepper_api_after(void *f)
{
	struct stepper_api_fixture *fixture = f;
	(void)stepper_enable(fixture->dev, false);
}

ZTEST_F(stepper_api, test_micro_step_res_set)
{
	(void)stepper_set_micro_step_res(fixture->dev, fixture->test_ms_resolution);
	enum stepper_micro_step_resolution res;
	(void)stepper_get_micro_step_res(fixture->dev, &res);
	zassert_equal(res, fixture->test_ms_resolution,
		      "Micro step resolution not set correctly, should be %d but is %d",
		      fixture->test_ms_resolution, res);
}

ZTEST_F(stepper_api, test_micro_step_res_set_incorrect_value_detection)
{
	int ret;

	ret = stepper_set_micro_step_res(fixture->dev, fixture->incorrect_ms_resoltion);
	zassert_equal(ret, -EINVAL, "Command should fail with error %d but returned %d", -EINVAL,
		      ret);
}

ZTEST_F(stepper_api, test_set_microstep_interval_zero_value)
{
	int32_t ret;

	(void)stepper_enable(fixture->dev, true);
	ret = stepper_set_microstep_interval(fixture->dev, 0);
	zassert_equal(ret, -EINVAL,
		      "Set_microstep_interval should fail with error code %d but returned %d",
		      -EINVAL, ret);
}

ZTEST_F(stepper_api, test_actual_position_set)
{
	int32_t pos = 100u;
	(void)stepper_set_reference_position(fixture->dev, pos);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 100u, "Actual position should be %u but is %u", 100u, pos);
}

ZTEST_F(stepper_api, test_is_not_moving_when_disabled)
{
	int32_t steps = 100;
	bool moving = true;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_enable(fixture->dev, false);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after being disabled");
}

ZTEST_F(stepper_api, test_position_not_updating_when_disabled)
{
	int32_t steps = 1000;
	int32_t position_1;
	int32_t position_2;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_enable(fixture->dev, false);
	(void)stepper_get_actual_position(fixture->dev, &position_1);
	k_msleep(100);
	(void)stepper_get_actual_position(fixture->dev, &position_2);
	zassert_equal(position_2, position_1,
		      "Actual position should not have changed from %d but is %d", position_1,
		      position_2);
}

ZTEST_F(stepper_api, test_is_not_moving_when_reenabled_after_movement)
{
	int32_t steps = 1000;
	bool moving = true;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_enable(fixture->dev, false);
	(void)k_msleep(100);
	(void)stepper_enable(fixture->dev, true);
	(void)k_msleep(100);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after being reenabled");
}
ZTEST_F(stepper_api, test_position_not_updating_when_reenabled_after_movement)
{
	int32_t steps = 1000;
	int32_t position_1;
	int32_t position_2;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_enable(fixture->dev, false);
	(void)stepper_get_actual_position(fixture->dev, &position_1);
	(void)k_msleep(100);
	(void)stepper_enable(fixture->dev, true);
	(void)k_msleep(100);
	(void)stepper_get_actual_position(fixture->dev, &position_2);
	zassert_equal(position_2, position_1,
		      "Actual position should not have changed from %d but is %d", position_1,
		      position_2);
}

ZTEST_F(stepper_api, test_move_to_identical_current_and_target_position)
{
	int32_t pos = 0;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);
	(void)k_poll(&stepper_event, 1, K_SECONDS(1));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 0, "Target position should not have changed from %d but is %d", 0, pos);
}

ZTEST_F(stepper_api, test_move_to_is_moving_true_while_moving)
{
	int32_t pos = 1000;
	bool moving = false;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving while moving");
}

ZTEST_F(stepper_api, test_move_to_is_moving_false_when_completed)
{
	bool moving = false;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, fixture->test_steps);
	(void)k_poll(&stepper_event, 1, K_SECONDS(5));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after finishing");
}

ZTEST_F(stepper_api, test_move_to_no_movement_when_disabled)
{
	int32_t curr_pos;
	int32_t ret;

	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_enable(fixture->dev, false);

	ret = stepper_move_to(fixture->dev, fixture->test_steps);
	zassert_equal(ret, -ECANCELED, "Move_to should fail with error code %d but returned %d",
		      -ECANCELED, ret);
	(void)stepper_get_actual_position(fixture->dev, &curr_pos);
	zassert_equal(curr_pos, 0, "Current position should not have changed from %d but is %d", 0,
		      curr_pos);
}

ZTEST_F(stepper_api, test_move_by_zero_steps_no_movement)
{
	int32_t steps = 0;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);
	(void)k_poll(&stepper_event, 1, K_SECONDS(1));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 0, "Target position should be %d but is %d", 0, steps);
}

ZTEST_F(stepper_api, test_move_by_is_moving_true_while_moving)
{
	int32_t steps = 1000;
	bool moving = false;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
}

ZTEST_F(stepper_api, test_move_by_is_moving_false_when_completed)
{
	bool moving = true;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, fixture->test_steps);
	(void)k_poll(&stepper_event, 1, K_SECONDS(5));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after completion");
}

ZTEST_F(stepper_api, test_move_by_no_movement_when_disabled)
{
	int32_t steps = 100;
	int32_t curr_pos;
	int32_t ret;

	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_enable(fixture->dev, false);

	ret = stepper_move_by(fixture->dev, steps);
	zassert_equal(ret, -ECANCELED, "Move_by should fail with error code %d but returned %d",
		      -ECANCELED, ret);
	(void)stepper_get_actual_position(fixture->dev, &curr_pos);
	zassert_equal(curr_pos, 0, "Current position should not have changed from %d but is %d", 0,
		      curr_pos);
}

ZTEST_F(stepper_api, test_run_is_moving_true_when_step_interval_greater_zero)
{
	bool moving = false;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
	(void)stepper_enable(fixture->dev, false);
}

ZTEST_F(stepper_api, test_run_no_movement_when_disabled)
{
	int32_t steps;
	int32_t ret;

	(void)stepper_enable(fixture->dev, false);
	(void)stepper_set_microstep_interval(fixture->dev, fixture->test_interval);

	ret = stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	zassert_equal(ret, -ECANCELED, "Run should fail with error code %d but returned %d",
		      -ECANCELED, ret);
	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 0, "Current position should not have changed from %d but is %d", 0,
		      steps);
}

ZTEST_SUITE(stepper_api, NULL, stepper_api_setup, stepper_api_before, stepper_api_after, NULL);

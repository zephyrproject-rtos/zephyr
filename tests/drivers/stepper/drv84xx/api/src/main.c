/*
 * Copyright 2024 Navimatix GmbH
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

struct drv84xx_api_fixture {
	const struct device *dev;
	stepper_event_callback_t callback;
};

struct k_poll_signal stepper_signal;
struct k_poll_event stepper_event;

static void drv84xx_api_print_event_callback(const struct device *dev, enum stepper_event event,
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

static void *drv84xx_api_setup(void)
{
	static struct drv84xx_api_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_ALIAS(stepper)),
		.callback = drv84xx_api_print_event_callback,
	};

	k_poll_signal_init(&stepper_signal);
	k_poll_event_init(&stepper_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &stepper_signal);

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void drv84xx_api_before(void *f)
{
	struct drv84xx_api_fixture *fixture = f;
	(void)stepper_set_reference_position(fixture->dev, 0);
	(void)stepper_set_micro_step_res(fixture->dev, 1);
	k_poll_signal_reset(&stepper_signal);
}

static void drv84xx_api_after(void *f)
{
	struct drv84xx_api_fixture *fixture = f;
	(void)stepper_disable(fixture->dev);
}

ZTEST_F(drv84xx_api, test_micro_step_res_set)
{
	(void)stepper_set_micro_step_res(fixture->dev, 4);
	enum stepper_micro_step_resolution res;
	(void)stepper_get_micro_step_res(fixture->dev, &res);
	zassert_equal(res, 4, "Micro step resolution not set correctly, should be %d but is %d", 4,
		      res);
}

ZTEST_F(drv84xx_api, test_actual_position_set)
{
	int32_t pos = 100u;
	(void)stepper_set_reference_position(fixture->dev, pos);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 100u, "Actual position should be %u but is %u", 100u, pos);
}

ZTEST_F(drv84xx_api, test_is_not_moving_when_disabled)
{
	int32_t steps = 100;
	bool moving = true;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_disable(fixture->dev);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after being disabled");
}

ZTEST_F(drv84xx_api, test_position_not_updating_when_disabled)
{
	int32_t steps = 1000;
	int32_t position_1 = 0;
	int32_t position_2 = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_disable(fixture->dev);
	(void)stepper_get_actual_position(fixture->dev, &position_1);
	k_msleep(100);
	(void)stepper_get_actual_position(fixture->dev, &position_2);
	zassert_equal(position_2, position_1,
		      "Actual position should not have changed from %d but is %d", position_1,
		      position_2);
}

ZTEST_F(drv84xx_api, test_is_not_moving_when_reenabled_after_movement)
{
	int32_t steps = 1000;
	bool moving = true;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_disable(fixture->dev);
	(void)k_msleep(100);
	(void)stepper_enable(fixture->dev);
	(void)k_msleep(100);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after being reenabled");
}
ZTEST_F(drv84xx_api, test_position_not_updating_when_reenabled_after_movement)
{
	int32_t steps = 1000;
	int32_t position_1 = 0;
	int32_t position_2 = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_disable(fixture->dev);
	(void)stepper_get_actual_position(fixture->dev, &position_1);
	(void)k_msleep(100);
	(void)stepper_enable(fixture->dev);
	(void)k_msleep(100);
	(void)stepper_get_actual_position(fixture->dev, &position_2);
	zassert_equal(position_2, position_1,
		      "Actual position should not have changed from %d but is %d", position_1,
		      position_2);
}

ZTEST_F(drv84xx_api, test_move_to_positive_direction_movement)
{
	int32_t pos = 50;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);
	(void)k_poll(&stepper_event, 1, K_SECONDS(5));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 50u, "Target position should be %d but is %d", 50u, pos);
}

ZTEST_F(drv84xx_api, test_move_to_negative_direction_movement)
{
	int32_t pos = -50;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);
	(void)k_poll(&stepper_event, 1, K_SECONDS(5));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, -50, "Target position should be %d but is %d", -50, pos);
}

ZTEST_F(drv84xx_api, test_move_to_identical_current_and_target_position)
{
	int32_t pos = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);
	(void)k_poll(&stepper_event, 1, K_SECONDS(5));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 0, "Target position should not have changed from %d but is %d", 0, pos);
}

ZTEST_F(drv84xx_api, test_move_to_is_moving_true_while_moving)
{
	int32_t pos = 50;
	bool moving = false;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving while moving");
}

ZTEST_F(drv84xx_api, test_move_to_is_moving_false_when_completed)
{
	int32_t pos = 50;
	bool moving = false;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);
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

ZTEST_F(drv84xx_api, test_move_to_no_movement_when_disabled)
{
	int32_t pos = 50;
	int32_t curr_pos = 50;
	int32_t ret = 0;

	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_disable(fixture->dev);

	ret = stepper_move_to(fixture->dev, pos);
	zassert_equal(ret, -ECANCELED, "Move_to should fail with error code %d but returned %d",
		      -ECANCELED, ret);
	(void)stepper_get_actual_position(fixture->dev, &curr_pos);
	zassert_equal(curr_pos, 0, "Current position should not have changed from %d but is %d", 0,
		      curr_pos);
}

ZTEST_F(drv84xx_api, test_move_by_positive_step_count)
{
	int32_t steps = 50;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);
	(void)k_poll(&stepper_event, 1, K_SECONDS(5));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 50u, "Target position should be %d but is %d", 50u, steps);
}

ZTEST_F(drv84xx_api, test_move_by_negative_step_count)
{
	int32_t steps = -50;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);
	(void)k_poll(&stepper_event, 1, K_SECONDS(5));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, -50, "Target position should be %d but is %d", -50, steps);
}

ZTEST_F(drv84xx_api, test_move_by_zero_steps_no_movement)
{
	int32_t steps = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);
	(void)k_poll(&stepper_event, 1, K_SECONDS(5));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&stepper_signal, &signaled, &result);
	zassert_equal(signaled, 1, "No event detected");
	zassert_equal(result, STEPPER_EVENT_STEPS_COMPLETED,
		      "Event was not STEPPER_EVENT_STEPS_COMPLETED event");
	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 0, "Target position should be %d but is %d", 0, steps);
}

ZTEST_F(drv84xx_api, test_move_by_zero_step_interval)
{
	int32_t steps = 100;
	int32_t ret = 0;
	int32_t pos = 100;

	(void)stepper_enable(fixture->dev);
	(void)stepper_disable(fixture->dev);
	ret = stepper_move_by(fixture->dev, steps);

	zassert_not_equal(ret, 0, "Command should fail with an error code, but returned 0");
	k_msleep(100);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 0, "Target position should not have changed from %d but is %d", 0, pos);
}

ZTEST_F(drv84xx_api, test_move_by_is_moving_true_while_moving)
{
	int32_t steps = 50;
	bool moving = false;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
}

ZTEST_F(drv84xx_api, test_move_by_is_moving_false_when_completed)
{
	int32_t steps = 50;
	bool moving = true;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);
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

ZTEST_F(drv84xx_api, test_move_by_no_movement_when_disabled)
{
	int32_t steps = 100;
	int32_t curr_pos = 100;
	int32_t ret = 0;

	(void)stepper_set_microstep_interval(fixture->dev, 20000000);
	(void)stepper_disable(fixture->dev);

	ret = stepper_move_by(fixture->dev, steps);
	zassert_equal(ret, -ECANCELED, "Move_by should fail with error code %d but returned %d",
		      -ECANCELED, ret);
	(void)stepper_get_actual_position(fixture->dev, &curr_pos);
	zassert_equal(curr_pos, 0, "Current position should not have changed from %d but is %d", 0,
		      curr_pos);
}

ZTEST_F(drv84xx_api, test_run_positive_direction_correct_position)
{
	uint64_t step_interval = 20000000;
	int32_t steps = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, step_interval);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_busy_wait(110000);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_true(IN_RANGE(steps, 4, 6), "Current position should be between 4 and 6 but is %d",
		     steps);
}

ZTEST_F(drv84xx_api, test_run_negative_direction_correct_position)
{
	uint64_t step_interval = 20000000;
	int32_t steps = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, step_interval);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_busy_wait(110000);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_true(IN_RANGE(steps, -6, 4),
		     "Current position should be between -6 and -4 but is %d", steps);
}

ZTEST_F(drv84xx_api, test_run_zero_step_interval_correct_position)
{
	uint64_t step_interval = 0;
	int32_t steps = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, step_interval);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(100);

	zassert_equal(steps, 0, "Current position should not have changed from %d but is %d", 0,
		      steps);
}

ZTEST_F(drv84xx_api, test_run_is_moving_true_when_step_interval_greater_zero)
{
	uint64_t step_interval = 20000000;
	bool moving = false;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, step_interval);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
	(void)stepper_disable(fixture->dev);
}

ZTEST_F(drv84xx_api, test_run_no_movement_when_disabled)
{
	uint64_t step_interval = 20000000;
	int32_t steps = 50;
	int32_t ret = 0;

	(void)stepper_disable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, step_interval);

	ret = stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	zassert_equal(ret, -ECANCELED, "Run should fail with error code %d but returned %d",
		      -ECANCELED, ret);
	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 0, "Current position should not have changed from %d but is %d", 0,
		      steps);
}

ZTEST_SUITE(drv84xx_api, NULL, drv84xx_api_setup, drv84xx_api_before, drv84xx_api_after, NULL);

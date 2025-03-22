/*
 * SPDX-FileCopyrightText: Copyright 2025 Navimatix GmbH
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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_api, CONFIG_STEPPER_LOG_LEVEL);

#define TEST_INTERVAL (NSEC_PER_SEC / CONFIG_USTEPS_PER_SECOND)
#define RUNTIME                                                                                    \
	(K_MSEC(1000 +                                                                             \
		DIV_ROUND_UP(MSEC_PER_SEC, CONFIG_USTEPS_PER_SECOND))) /* 1sec + time 1 step*/

struct stepper_const_velocity_fixture {
	const struct device *dev;
	stepper_event_callback_t callback;
	uint32_t test_steps;
};

struct k_poll_signal stepper_signal;
struct k_poll_event stepper_event;
void *user_data_received;

#define POLL_AND_CHECK_SIGNAL(signal, event, expected_event, timeout)                              \
	({                                                                                         \
		(void)k_poll(&(event), 1, timeout);                                                \
		unsigned int signaled;                                                             \
		int result;                                                                        \
		k_poll_signal_check(&(signal), &signaled, &result);                                \
		zassert_equal(signaled, 1, "Signal not set");                                      \
		zassert_equal(result, (expected_event), "Signal not set");                         \
	})

static void stepper_const_velocity_print_event_callback(const struct device *dev,
							enum stepper_event event, void *user_data)
{
	const struct device *dev_callback = user_data;

	user_data_received = user_data;

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
	case STEPPER_EVENT_STOPPED:
		k_poll_signal_raise(&stepper_signal, STEPPER_EVENT_STOPPED);
		break;
	default:
		break;
	}

	LOG_DBG("Event %d, %s called for %s, expected for %s", event, __func__, dev_callback->name,
		dev->name);
}

static void *stepper_const_velocity_setup(void)
{
	static struct stepper_const_velocity_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_ALIAS(stepper)),
		.callback = stepper_const_velocity_print_event_callback,
		.test_steps = CONFIG_USTEPS_PER_SECOND,
	};

	k_poll_signal_init(&stepper_signal);
	k_poll_event_init(&stepper_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &stepper_signal);

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void stepper_const_velocity_before(void *f)
{
	struct stepper_const_velocity_fixture *fixture = f;
	(void)stepper_set_reference_position(fixture->dev, 0);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_INTERVAL);
	k_poll_signal_reset(&stepper_signal);
}

static void stepper_const_velocity_after(void *f)
{
	struct stepper_const_velocity_fixture *fixture = f;
	(void)stepper_enable(fixture->dev, false);
}

ZTEST_F(stepper_const_velocity, test_move_to_positive_direction_movement)
{
	int32_t pos;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_move_to(fixture->dev, fixture->test_steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      RUNTIME);

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, fixture->test_steps, "Target position should be %d but is %d",
		      fixture->test_steps, pos);
}

ZTEST_F(stepper_const_velocity, test_move_to_negative_direction_movement)
{
	int32_t pos;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_move_to(fixture->dev, -fixture->test_steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      RUNTIME);

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, -fixture->test_steps, "Target position should be %d but is %d",
		      -fixture->test_steps, pos);
}

ZTEST_F(stepper_const_velocity, test_move_to_positive_direction_velocity)
{
	int32_t pos;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_move_to(fixture->dev, fixture->test_steps);
	k_msleep(500);

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_true(IN_RANGE(pos, DIV_ROUND_UP(fixture->test_steps, 2) - 1,
			      DIV_ROUND_UP(fixture->test_steps, 2) + 1),
		     "Target position should be between %d and %d but is %d",
		     DIV_ROUND_UP(fixture->test_steps, 2) - 1,
		     DIV_ROUND_UP(fixture->test_steps, 2) + 1, pos);
}

ZTEST_F(stepper_const_velocity, test_move_to_negative_direction_velocity)
{
	int32_t pos;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_move_to(fixture->dev, -fixture->test_steps);
	k_msleep(500);

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_true(IN_RANGE(pos, -DIV_ROUND_UP(fixture->test_steps, 2) - 1,
			      -DIV_ROUND_UP(fixture->test_steps, 2) + 1),
		     "Target position should be between %d and %d but is %d",
		     -DIV_ROUND_UP(fixture->test_steps, 2) - 1,
		     -DIV_ROUND_UP(fixture->test_steps, 2) + 1, pos);
}

ZTEST_F(stepper_const_velocity, test_move_by_positive_step_count)
{
	int32_t steps;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_move_by(fixture->dev, fixture->test_steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      RUNTIME);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, fixture->test_steps, "Target position should be %d but is %d",
		      fixture->test_steps, steps);
}

ZTEST_F(stepper_const_velocity, test_move_by_negative_step_count)
{
	int32_t steps;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_move_by(fixture->dev, -fixture->test_steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      RUNTIME);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, -fixture->test_steps, "Target position should be %d but is %d",
		      -fixture->test_steps, steps);
}

ZTEST_F(stepper_const_velocity, test_move_by_positive_direction_velocity)
{
	int32_t pos;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_move_by(fixture->dev, fixture->test_steps);
	k_msleep(500);

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_true(IN_RANGE(pos, DIV_ROUND_UP(fixture->test_steps, 2) - 1,
			      DIV_ROUND_UP(fixture->test_steps, 2) + 1),
		     "Target position should be between %d and %d but is %d",
		     DIV_ROUND_UP(fixture->test_steps, 2) - 1,
		     DIV_ROUND_UP(fixture->test_steps, 2) + 1, pos);
}

ZTEST_F(stepper_const_velocity, test_move_by_negative_direction_velocity)
{
	int32_t pos;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_move_by(fixture->dev, -fixture->test_steps);
	k_msleep(500);

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_true(IN_RANGE(pos, -DIV_ROUND_UP(fixture->test_steps, 2) - 1,
			      -DIV_ROUND_UP(fixture->test_steps, 2) + 1),
		     "Target position should be between %d and %d but is %d",
		     -DIV_ROUND_UP(fixture->test_steps, 2) - 1,
		     -DIV_ROUND_UP(fixture->test_steps, 2) + 1, pos);
}

ZTEST_F(stepper_const_velocity, test_run_positive_direction_correct_position)
{
	int32_t steps;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(1000);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_true(IN_RANGE(steps, fixture->test_steps - 1, fixture->test_steps + 1),
		     "Current position should be between %u and %u but is %d",
		     fixture->test_steps - 1, fixture->test_steps + 1, steps);
}

ZTEST_F(stepper_const_velocity, test_run_negative_direction_correct_position)
{
	int32_t steps;

	(void)stepper_enable(fixture->dev, true);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_msleep(1000);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_true(IN_RANGE(steps, -fixture->test_steps - 1, -fixture->test_steps + 1),
		     "Current position should be between %d and %d but is %d",
		     -fixture->test_steps - 1, -fixture->test_steps + 1, steps);
}

ZTEST_SUITE(stepper_const_velocity, NULL, stepper_const_velocity_setup,
	    stepper_const_velocity_before, stepper_const_velocity_after, NULL);

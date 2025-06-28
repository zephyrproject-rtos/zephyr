/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/stepper.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_api, CONFIG_STEPPER_LOG_LEVEL);

struct stepper_fixture {
	const struct device *dev;
	stepper_event_callback_t callback;
};

struct k_poll_signal stepper_signal;
struct k_poll_event stepper_event;
void *user_data_received;

#define TEST_STEP_INTERVAL (20 * USEC_PER_SEC)

#define POLL_AND_CHECK_SIGNAL(signal, event, expected_event, timeout)                              \
	({                                                                                         \
		do {                                                                               \
			(void)k_poll(&(event), 1, timeout);                                        \
			unsigned int signaled;                                                     \
			int result;                                                                \
			k_poll_signal_check(&(signal), &signaled, &result);                        \
			zassert_equal(signaled, 1, "Signal not set");                              \
			zassert_equal(result, (expected_event), "Signal not set");                 \
		} while (0);                                                                       \
	})

static void stepper_print_event_callback(const struct device *dev, enum stepper_event event,
					 void *user_data)
{
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
}

static void *stepper_setup(void)
{
	static struct stepper_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_ALIAS(stepper)),
		.callback = stepper_print_event_callback,
	};

	k_poll_signal_init(&stepper_signal);
	k_poll_event_init(&stepper_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &stepper_signal);

	zassert_not_null(fixture.dev);
	(void)stepper_enable(fixture.dev);
	return &fixture;
}

static void stepper_before(void *f)
{
	struct stepper_fixture *fixture = f;
	(void)stepper_set_reference_position(fixture->dev, 0);
	(void)stepper_set_micro_step_res(fixture->dev, DT_PROP(DT_ALIAS(stepper), micro_step_res));

	k_poll_signal_reset(&stepper_signal);

	user_data_received = NULL;
}

static void stepper_after(void *f)
{
	struct stepper_fixture *fixture = f;
	(void)stepper_stop(fixture->dev);
	(void)stepper_disable(fixture->dev);
}

ZTEST_SUITE(stepper, NULL, stepper_setup, stepper_before, stepper_after, NULL);

ZTEST_F(stepper, test_set_micro_step_res_invalid)
{
	int ret = stepper_set_micro_step_res(fixture->dev, 127);

	zassert_equal(ret, -EINVAL, "Invalid micro step resolution should return -EINVAL");
}

ZTEST_F(stepper, test_set_micro_step_res_valid)
{
	int ret =
		stepper_set_micro_step_res(fixture->dev, CONFIG_STEPPER_TEST_MICROSTEP_RESOLUTION);
	zassert_equal(ret, 0, "Failed to set microstep resolution");

	enum stepper_micro_step_resolution res;
	(void)stepper_get_micro_step_res(fixture->dev, &res);
	zassert_equal(res, CONFIG_STEPPER_TEST_MICROSTEP_RESOLUTION,
		      "Micro step resolution not set correctly, should be %d but is %d",
		      CONFIG_STEPPER_TEST_MICROSTEP_RESOLUTION, res);
}

ZTEST_F(stepper, test_get_micro_step_res)
{
	enum stepper_micro_step_resolution res;
	(void)stepper_get_micro_step_res(fixture->dev, &res);
	zassert_equal(res, DT_PROP(DT_ALIAS(stepper), micro_step_res),
		      "Micro step resolution not set correctly");
}

ZTEST_F(stepper, test_set_micro_step_interval_invalid_zero)
{
	int err = stepper_set_microstep_interval(fixture->dev, 0);
	if (err == -ENOSYS) {
		ztest_test_skip();
	}
	zassert_equal(err, -EINVAL, "ustep interval cannot be zero");
}

ZTEST_F(stepper, test_set_reference_position)
{
	int32_t pos = 100u;
	int ret;

	ret = stepper_set_reference_position(fixture->dev, pos);
	zassert_equal(ret, 0, "Failed to set reference position");

	ret = stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(ret, 0, "Failed to get actual position");
	zassert_equal(pos, 100u, "Actual position should be %u but is %u", 100u, pos);
}

ZTEST_F(stepper, test_stop)
{
	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);

	/* Run the stepper in positive direction */
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);

	/* Stop the stepper */
	int ret = stepper_stop(fixture->dev);
	bool is_moving;

	if (ret == 0) {
		POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED,
				      K_NO_WAIT);
		zassert_equal(user_data_received, fixture->dev, "User data not received");

		/* Check if the stepper is stopped */
		stepper_is_moving(fixture->dev, &is_moving);
		zassert_equal(is_moving, false, "Stepper is still moving");
	} else if (ret == -ENOSYS) {
		stepper_is_moving(fixture->dev, &is_moving);
		zassert_equal(is_moving, true,
			      "Stepper should be moving since stop is not implemented");
	} else {
		zassert_unreachable("Stepper stop failed");
	}
}

ZTEST_F(stepper, test_move_to_positive_direction_movement)
{
	int32_t pos = 20u;

	Z_TEST_SKIP_IFDEF(CONFIG_STEPPER_TEST_ACCELERATION);

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);
	(void)stepper_move_to(fixture->dev, pos);

	POLL_AND_CHECK_SIGNAL(
		stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
		K_MSEC(pos * (20 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT)));

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 20u, "Target position should be %d but is %d", 20u, pos);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_to_negative_direction_movement)
{
	int32_t pos = -20u;

	Z_TEST_SKIP_IFDEF(CONFIG_STEPPER_TEST_ACCELERATION);

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);
	(void)stepper_move_to(fixture->dev, pos);

	POLL_AND_CHECK_SIGNAL(
		stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
		K_MSEC(-pos * (20 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT)));

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, -20u, "Target position should be %d but is %d", -20u, pos);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_to_is_moving_false_when_completed)
{
	int32_t pos = 20;
	bool moving;

	Z_TEST_SKIP_IFDEF(CONFIG_STEPPER_TEST_ACCELERATION);

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);

	POLL_AND_CHECK_SIGNAL(
		stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
		K_MSEC(pos * (20 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT)));

	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after finishing");
}

ZTEST_F(stepper, test_move_to_identical_current_and_target_position)
{
	int32_t pos = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);
	(void)stepper_move_to(fixture->dev, pos);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_NO_WAIT);

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 0, "Target position should not have changed from %d but is %d", 0, pos);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_to_is_moving_true_while_moving)
{
	int32_t pos = 50;
	bool moving = false;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_to(fixture->dev, pos);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving while moving");
}

ZTEST_F(stepper, test_move_by_positive_step_count)
{
	int32_t steps = 20;

	Z_TEST_SKIP_IFDEF(CONFIG_STEPPER_TEST_ACCELERATION);

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);
	(void)stepper_move_by(fixture->dev, steps);

	POLL_AND_CHECK_SIGNAL(
		stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
		K_MSEC(steps * (20 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT)));

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 20u, "Target position should be %d but is %d", 20u, steps);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_by_negative_step_count)
{
	int32_t steps = -20;

	Z_TEST_SKIP_IFDEF(CONFIG_STEPPER_TEST_ACCELERATION);

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);
	(void)stepper_move_by(fixture->dev, steps);

	POLL_AND_CHECK_SIGNAL(
		stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
		K_MSEC(-steps * (20 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT)));

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, -20u, "Target position should be %d but is %d", -20u, steps);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_by_is_moving_false_when_completed)
{
	int32_t steps = 20;
	bool moving;

	Z_TEST_SKIP_IFDEF(CONFIG_STEPPER_TEST_ACCELERATION);

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);

	POLL_AND_CHECK_SIGNAL(
		stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
		K_MSEC(steps * (20 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT)));

	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after completion");
}

ZTEST_F(stepper, test_move_by_zero_steps_no_movement)
{
	int32_t steps = 0;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_NO_WAIT);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 0, "Target position should be %d but is %d", 0, steps);
}

ZTEST_F(stepper, test_move_by_is_moving_true_while_moving)
{
	int32_t steps = 20;
	bool moving = false;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, NULL);
	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
}

ZTEST_F(stepper, test_run_positive_direction_correct_position)
{
	int32_t steps = 0;

	Z_TEST_SKIP_IFDEF(CONFIG_STEPPER_TEST_ACCELERATION);

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(100 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_true(IN_RANGE(steps, 4, 6), "Current position should be between 4 and 6 but is %d",
		     steps);
}

ZTEST_F(stepper, test_run_negative_direction_correct_position)
{
	int32_t steps = 0;

	Z_TEST_SKIP_IFDEF(CONFIG_STEPPER_TEST_ACCELERATION);

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_msleep(100 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_true(IN_RANGE(steps, -6, -4),
		     "Current position should be between -6 and -4 but is %d", steps);
}

ZTEST_F(stepper, test_run_is_moving_true_while_moving)
{
	bool moving = false;

	(void)stepper_enable(fixture->dev);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_STEP_INTERVAL);
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
}

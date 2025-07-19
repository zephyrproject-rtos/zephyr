/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdlib.h>
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

#define STEPPER_TIMEOUT(steps)                                                                     \
	((steps * CONFIG_STEPPER_TEST_MICROSTEP_INTERVAL / NSEC_PER_MSEC) *                        \
	 (100 + CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT) / 100)

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

	LOG_DBG("Event %d, %s called for %s, expected for %s\n", event, __func__,
		dev_callback != NULL ? dev_callback->name : "", dev->name);
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
	return &fixture;
}

static void stepper_before(void *f)
{
	struct stepper_fixture *fixture = f;
	(void)stepper_set_reference_position(fixture->dev, 0);
	(void)stepper_set_micro_step_res(fixture->dev, DT_PROP(DT_ALIAS(stepper), micro_step_res));
	(void)stepper_set_microstep_interval(fixture->dev, CONFIG_STEPPER_TEST_MICROSTEP_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);

	k_poll_signal_reset(&stepper_signal);

	user_data_received = NULL;
	zassert_not_equal(stepper_enable(fixture->dev), -EIO, "Failed to enable device");
}

static void stepper_after(void *f)
{
	struct stepper_fixture *fixture = f;
	(void)stepper_stop(fixture->dev);
	zassert_not_equal(stepper_disable(fixture->dev), -EIO, "Failed to disable device");
}

ZTEST_SUITE(stepper, NULL, stepper_setup, stepper_before, stepper_after, NULL);

ZTEST_F(stepper, test_set_micro_step_res_invalid)
{
	int ret = stepper_set_micro_step_res(fixture->dev, 127);

	zassert_equal(ret, -EINVAL, "Invalid micro step resolution should return -EINVAL");
}

ZTEST_F(stepper, test_set_micro_step_res_valid)
{
	int ret;

	ret = stepper_set_micro_step_res(fixture->dev, CONFIG_STEPPER_TEST_MICROSTEP_RESOLUTION);
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

ZTEST_F(stepper, test_set_micro_step_interval_valid)
{
	int err = stepper_set_microstep_interval(fixture->dev,
						 CONFIG_STEPPER_TEST_MICROSTEP_INTERVAL);
	if (err == -ENOSYS) {
		ztest_test_skip();
	}
	zassert_equal(err, 0, "Ustep interval could not be set");
}

ZTEST_F(stepper, test_set_event_callback_valid)
{
	/* The only possible error code is -ENOSYS, with other words, the functionality is not
	 * implemented. While this is valid behaviour for the api, this will cause cauntless other
	 * tests of this test suite to fail.
	 */
	int err = stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);

	zassert_equal(err, 0,
		      "Event callback could not be set. This is valid behaviour but will cause "
		      "most other tests of this test suite to fail.");
}

ZTEST_F(stepper, test_set_reference_position)
{
	int32_t pos = 100;
	int ret;

	ret = stepper_set_reference_position(fixture->dev, pos);
	zassert_equal(ret, 0, "Failed to set reference position");

	ret = stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(ret, 0, "Failed to get actual position");
	zassert_equal(pos, 100, "Actual position should be %u but is %u", 100, pos);
}

ZTEST_F(stepper, test_stop)
{
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
	int32_t pos = 10;

	(void)stepper_move_to(fixture->dev, pos);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(STEPPER_TIMEOUT(pos)));

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 10, "Target position should be %d but is %d", 10, pos);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_to_negative_direction_movement)
{
	int32_t pos = -10;

	(void)stepper_move_to(fixture->dev, pos);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(STEPPER_TIMEOUT(abs(pos))));

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, -10, "Target position should be %d but is %d", -10, pos);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_to_is_moving_false_when_completed)
{
	int32_t pos = 10;
	bool moving;

	(void)stepper_move_to(fixture->dev, pos);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(STEPPER_TIMEOUT(pos)));

	k_msleep(1);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after finishing");
}

ZTEST_F(stepper, test_move_to_identical_current_and_target_position)
{
	int32_t pos = 0;

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

	(void)stepper_move_to(fixture->dev, pos);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving while moving");
}

ZTEST_F(stepper, test_move_by_positive_step_count)
{
	int32_t steps = 10;

	(void)stepper_move_by(fixture->dev, steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(STEPPER_TIMEOUT(steps)));

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 10, "Target position should be %d but is %d", 10, steps);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_by_negative_step_count)
{
	int32_t steps = -10;

	(void)stepper_move_by(fixture->dev, steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(STEPPER_TIMEOUT(abs(steps))));

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, -10, "Target position should be %d but is %d", -10, steps);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_move_by_is_moving_false_when_completed)
{
	int32_t steps = 20;
	bool moving;

	(void)stepper_move_by(fixture->dev, steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(STEPPER_TIMEOUT(steps)));

	k_msleep(1);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after completion");
}

ZTEST_F(stepper, test_move_by_zero_steps_no_movement)
{
	int32_t steps = 0;

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

	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
}

ZTEST_F(stepper, test_run_positive_direction_correct_position)
{
	int32_t steps = 0;
	int32_t target_position = 5;

	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	k_msleep(target_position * CONFIG_STEPPER_TEST_MICROSTEP_INTERVAL / NSEC_PER_MSEC);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	int32_t min = target_position -
		      DIV_ROUND_UP((CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT * 5), 100);
	int32_t max = target_position +
		      DIV_ROUND_UP((CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT * 5), 100);
	zassert_true(IN_RANGE(steps, min, max),
		     "Current position should be between %d and %d but is %d", min, max, steps);
}

ZTEST_F(stepper, test_run_negative_direction_correct_position)
{
	int32_t steps = 0;
	int32_t target_position = -5;

	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_NEGATIVE);
	k_msleep(abs(target_position) * CONFIG_STEPPER_TEST_MICROSTEP_INTERVAL / NSEC_PER_MSEC);

	(void)stepper_get_actual_position(fixture->dev, &steps);
	int32_t min = target_position -
		      DIV_ROUND_UP((CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT * 5), 100);
	int32_t max = target_position +
		      DIV_ROUND_UP((CONFIG_STEPPER_TEST_TIMING_TIMEOUT_TOLERANCE_PCT * 5), 100);

	zassert_true(IN_RANGE(steps, min, max),
		     "Current position should be between %d and %d but is %d", min, max, steps);
}

ZTEST_F(stepper, test_run_is_moving_true_while_moving)
{
	bool moving = false;

	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
}

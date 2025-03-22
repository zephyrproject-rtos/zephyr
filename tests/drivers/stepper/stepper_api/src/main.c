/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-FileCopyrightText: Copyright (c) 2025 Navimatix GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/stepper.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_api, CONFIG_STEPPER_LOG_LEVEL);

#define TEST_INTERVAL (NSEC_PER_SEC / CONFIG_USTEPS_PER_SECOND)

struct stepper_api_fixture {
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

static void stepper_api_print_event_callback(const struct device *dev, enum stepper_event event,
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

	LOG_DBG("Event %d, %s called for %s, expected for %s", event, __func__, dev_callback->name,
		dev->name);
}

static void *stepper_api_setup(void)
{
	static struct stepper_api_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_ALIAS(stepper)),
		.callback = stepper_api_print_event_callback,
		.test_steps = CONFIG_USTEPS_PER_SECOND,
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
	(void)stepper_set_micro_step_res(fixture->dev, CONFIG_BASE_MS_RESOLUTION);
	(void)stepper_set_microstep_interval(fixture->dev, TEST_INTERVAL);
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);
	(void)stepper_enable(fixture->dev, true);

	k_poll_signal_reset(&stepper_signal);

	user_data_received = NULL;
}

ZTEST_F(stepper_api, test_set_micro_step_res)
{
	(void)stepper_set_micro_step_res(fixture->dev, CONFIG_TEST_MS_RESOLUTION);
	enum stepper_micro_step_resolution res;
	(void)stepper_get_micro_step_res(fixture->dev, &res);
	zassert_equal(res, CONFIG_TEST_MS_RESOLUTION,
		      "Micro step resolution not set correctly, should be %d but is %d",
		      CONFIG_TEST_MS_RESOLUTION, res);
}

ZTEST_F(stepper_api, test_set_micro_step_res_incorrect)
{
	int ret = stepper_set_micro_step_res(fixture->dev, 127);

	zassert_equal(ret, -ENOTSUP, "Incorrect micro step resolution should return %d", -ENOTSUP);
}

ZTEST_F(stepper_api, test_get_micro_step_res)
{
	enum stepper_micro_step_resolution res;
	(void)stepper_get_micro_step_res(fixture->dev, &res);
	zassert_equal(res, DT_PROP(DT_ALIAS(stepper), micro_step_res),
		      "Micro step resolution not set correctly");
}

ZTEST_F(stepper_api, test_set_micro_step_interval_invalid_zero)
{
	int err = stepper_set_microstep_interval(fixture->dev, 0);

	zassert_equal(err, -EINVAL, "Ustep interval cannot be zero");
}

ZTEST_F(stepper_api, test_set_reference_position)
{
	int32_t pos = 100u;

	(void)stepper_set_reference_position(fixture->dev, pos);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 100u, "Actual position not set correctly");
}

ZTEST_F(stepper_api, test_stop)
{

	/* Run the stepper in positive direction */
	(void)stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);

	/* Stop the stepper */
	int ret = stepper_stop(fixture->dev);
	bool is_moving;

	if (ret == 0) {
		/* We do not know, how long it takes the driver to stop. Some finish immediately,
		 * others decelerate until they stop. 5s should be a sufficient upper bound.
		 */
		POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STOPPED,
				      K_SECONDS(5));
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

ZTEST_F(stepper_api, test_is_not_moving_when_disabled)
{
	int32_t steps = 100;
	bool moving = true;

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

	(void)stepper_move_to(fixture->dev, pos);

	/* Stepper events are often processed via the work queue, so it might take some time to
	 * process the event.
	 */
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(100));

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 0, "Target position should not have changed from %d but is %d", 0, pos);
}

ZTEST_F(stepper_api, test_move_to_is_moving_true_while_moving)
{
	int32_t pos = 1000;
	bool moving = false;

	(void)stepper_move_to(fixture->dev, pos);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving while moving");
}

ZTEST_F(stepper_api, test_move_to_is_moving_false_when_completed)
{
	bool moving = false;

	(void)stepper_move_to(fixture->dev, fixture->test_steps);

	/* timeout is set with 20% tolerance */
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(1200));

	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after finishing");
}

ZTEST_F(stepper_api, test_move_to_no_movement_when_disabled)
{
	int32_t curr_pos;
	int32_t ret;

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

	(void)stepper_move_by(fixture->dev, steps);

	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(100));

	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 0, "Target position should be %d but is %d", 0, steps);
}

ZTEST_F(stepper_api, test_move_by_is_moving_true_while_moving)
{
	int32_t steps = 1000;
	bool moving = false;

	(void)stepper_move_by(fixture->dev, steps);
	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_true(moving, "Driver should be in state is_moving");
}

ZTEST_F(stepper_api, test_move_by_is_moving_false_when_completed)
{
	bool moving = true;

	(void)stepper_move_by(fixture->dev, fixture->test_steps);

	/* timeout is set with 20% tolerance */
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(1200));

	(void)stepper_is_moving(fixture->dev, &moving);
	zassert_false(moving, "Driver should not be in state is_moving after completion");
}

ZTEST_F(stepper_api, test_move_by_no_movement_when_disabled)
{
	int32_t steps = 100;
	int32_t curr_pos;
	int32_t ret;

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
	ret = stepper_run(fixture->dev, STEPPER_DIRECTION_POSITIVE);
	zassert_equal(ret, -ECANCELED, "Run should fail with error code %d but returned %d",
		      -ECANCELED, ret);
	(void)stepper_get_actual_position(fixture->dev, &steps);
	zassert_equal(steps, 0, "Current position should not have changed from %d but is %d", 0,
		      steps);
}

ZTEST_SUITE(stepper_api, NULL, stepper_api_setup, stepper_api_before, NULL, NULL);

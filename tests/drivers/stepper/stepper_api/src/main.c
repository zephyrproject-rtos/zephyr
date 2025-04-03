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
		dev_callback->name, dev->name);
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

	k_poll_signal_reset(&stepper_signal);

	user_data_received = NULL;
}

ZTEST_SUITE(stepper, NULL, stepper_setup, stepper_before, NULL, NULL);

ZTEST_F(stepper, test_set_micro_step_res_incorrect)
{
	int ret = stepper_set_micro_step_res(fixture->dev, 127);

	zassert_equal(ret, -ENOTSUP, "Incorrect micro step resolution should return -ENOTSUP");
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

ZTEST_F(stepper, test_actual_position)
{
	int32_t pos = 100u;
	int ret;

	ret = stepper_set_reference_position(fixture->dev, pos);
	zassert_equal(ret, 0, "Failed to set reference position");

	ret = stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(ret, 0, "Failed to get actual position");
	zassert_equal(pos, 100u, "Actual position not set correctly");
}

ZTEST_F(stepper, test_target_position_w_fixed_step_interval)
{
	int32_t pos = 10u;
	int ret;

	ret = stepper_set_microstep_interval(fixture->dev, 100 * USEC_PER_SEC);

	if (ret == -ENOSYS) {
		ztest_test_skip();
	}
	/* Pass the function name as user data */
	(void)stepper_set_event_callback(fixture->dev, fixture->callback, (void *)fixture->dev);

	(void)stepper_move_to(fixture->dev, pos);

	/* timeout is set with 20% tolerance */
	POLL_AND_CHECK_SIGNAL(stepper_signal, stepper_event, STEPPER_EVENT_STEPS_COMPLETED,
			      K_MSEC(pos * 120));

	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 10u, "Target position should be %d but is %d", 10u, pos);
	zassert_equal(user_data_received, fixture->dev, "User data not received");
}

ZTEST_F(stepper, test_stop)
{
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

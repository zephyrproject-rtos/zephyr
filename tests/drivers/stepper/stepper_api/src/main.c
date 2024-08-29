/*
 * Copyright 2024 Jilay Sandeep Pandya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/stepper.h>

struct stepper_fixture {
	const struct device *dev;
};

static void *stepper_setup(void)
{
	static struct stepper_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(motor_1)),
	};

	zassert_not_null(fixture.dev);
	return &fixture;
}

static void stepper_before(void *f)
{
	struct stepper_fixture *fixture = f;
	(void)stepper_set_actual_position(fixture->dev, 0);
}

ZTEST_SUITE(stepper, NULL, stepper_setup, stepper_before, NULL, NULL);

ZTEST_F(stepper, test_micro_step_res)
{
	(void)stepper_set_micro_step_res(fixture->dev, 2);
	enum micro_step_resolution res;
	(void)stepper_get_micro_step_res(fixture->dev, &res);
	zassert_equal(res, 2, "Micro step resolution not set correctly");
}

ZTEST_F(stepper, test_actual_position)
{
	int32_t pos = 100u;
	(void)stepper_set_actual_position(fixture->dev, pos);
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 100u, "Actual position not set correctly");
}

ZTEST_F(stepper, test_target_position)
{
	int32_t pos = 100u;
	struct k_poll_signal signal;

	k_poll_signal_init(&signal);
	struct k_poll_event event;

	k_poll_event_init(&event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &signal);
	(void)stepper_set_max_velocity(fixture->dev, 100u);
	(void)stepper_set_target_position(fixture->dev, pos, &signal);
	(void)k_poll(&event, 1, K_SECONDS(2));
	unsigned int signaled;
	int result;

	k_poll_signal_check(&signal, &signaled, &result);
	zassert_equal(signaled, 1, "Signal not set");
	zassert_equal(result, STEPPER_SIGNAL_STEPS_COMPLETED, "Signal not set");
	(void)stepper_get_actual_position(fixture->dev, &pos);
	zassert_equal(pos, 100u, "Target position should be %d but is %d", 100u, pos);
}

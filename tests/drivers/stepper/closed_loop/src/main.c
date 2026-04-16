/*
 * Copyright (c) 2026 Contributors to the Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

#include "fake_encoder.h"
#include "stepper_fake.h"

static const struct device *cl_dev = DEVICE_DT_GET(DT_ALIAS(closed_loop_stepper));
static const struct device *enc_dev = DEVICE_DT_GET(DT_ALIAS(fake_encoder));

static volatile enum stepper_ctrl_event last_event;
static volatile bool event_received;
static volatile enum stepper_ctrl_event wait_for_event;
static struct k_sem event_sem;

static void event_callback(const struct device *dev, const enum stepper_ctrl_event event,
			    void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	last_event = event;
	event_received = true;

	/* Only signal semaphore for the event we're waiting for */
	if (event == wait_for_event) {
		k_sem_give(&event_sem);
	}
}

static void *closed_loop_test_setup(void)
{
	zassert_true(device_is_ready(cl_dev), "Closed-loop stepper device not ready");
	zassert_true(device_is_ready(enc_dev), "Fake encoder device not ready");

	k_sem_init(&event_sem, 0, 1);

	return NULL;
}

static void closed_loop_test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	event_received = false;
	wait_for_event = STEPPER_CTRL_EVENT_STOPPED;
	k_sem_reset(&event_sem);

	/* Reset encoder to zero */
	fake_encoder_set_rotation_degrees(enc_dev, 0);

	/* Reset stepper fakes */
	RESET_FAKE(fake_stepper_ctrl_move_by);
	RESET_FAKE(fake_stepper_ctrl_move_to);
	RESET_FAKE(fake_stepper_ctrl_stop);
	RESET_FAKE(fake_stepper_ctrl_run);
	RESET_FAKE(fake_stepper_ctrl_set_reference_position);

	/* Register callback */
	stepper_ctrl_set_event_cb(cl_dev, event_callback, NULL);

	/* Let control thread settle */
	k_msleep(10);
}

ZTEST_SUITE(stepper_closed_loop, NULL, closed_loop_test_setup, closed_loop_test_before, NULL, NULL);

ZTEST(stepper_closed_loop, test_device_ready)
{
	zassert_true(device_is_ready(cl_dev), "Closed-loop stepper should be ready");
}

ZTEST(stepper_closed_loop, test_move_to_delegates_to_inner)
{
	int ret;

	/*
	 * Set encoder to match the target so the loop settles quickly.
	 * Target: 100 microsteps = 100 * 3600/200 = 1800 encoder counts
	 * 1800 encoder counts = 1800 * 360 / 3600 = 180 degrees
	 */
	fake_encoder_set_rotation_degrees(enc_dev, 180);

	ret = stepper_ctrl_move_to(cl_dev, 100);
	zassert_equal(ret, 0, "move_to should return 0, got %d", ret);

	/* Verify inner controller received the move_to call */
	zassert_equal(fake_stepper_ctrl_move_to_fake.call_count, 1,
		      "Inner move_to should be called once");
	zassert_equal(fake_stepper_ctrl_move_to_fake.arg1_val, 100,
		      "Inner move_to should receive 100 microsteps");
}

ZTEST(stepper_closed_loop, test_move_to_settled_fires_event)
{
	int ret;

	/*
	 * Set encoder to exactly the target position so loop settles.
	 * 100 microsteps at 200 spr, 3600 cpr -> 1800 enc counts -> 180 degrees
	 */
	fake_encoder_set_rotation_degrees(enc_dev, 180);

	/* Wait specifically for STEPS_COMPLETED (ignore intermediate events) */
	wait_for_event = STEPPER_CTRL_EVENT_STEPS_COMPLETED;

	ret = stepper_ctrl_move_to(cl_dev, 100);
	zassert_equal(ret, 0, "move_to should succeed");

	/* Wait for settling (settling_count=3 cycles at 1ms = ~3ms + margin) */
	ret = k_sem_take(&event_sem, K_MSEC(500));
	zassert_equal(ret, 0, "Should receive event within timeout");
	zassert_equal(last_event, STEPPER_CTRL_EVENT_STEPS_COMPLETED,
		      "Should receive STEPS_COMPLETED event");
}

ZTEST(stepper_closed_loop, test_stop_fires_stopped_event)
{
	int ret;

	/* Start a move to a far target */
	fake_encoder_set_rotation_degrees(enc_dev, 0);
	ret = stepper_ctrl_move_to(cl_dev, 1000);
	zassert_equal(ret, 0, "move_to should succeed");

	k_msleep(5);

	/* Wait specifically for STOPPED */
	wait_for_event = STEPPER_CTRL_EVENT_STOPPED;

	/* Stop the motor */
	ret = stepper_ctrl_stop(cl_dev);
	zassert_equal(ret, 0, "stop should succeed");

	/* Verify stop was delegated */
	zassert_true(fake_stepper_ctrl_stop_fake.call_count >= 1,
		     "Inner stop should be called");

	/* Check for stopped event */
	ret = k_sem_take(&event_sem, K_MSEC(500));
	zassert_equal(ret, 0, "Should receive event within timeout");
	zassert_equal(last_event, STEPPER_CTRL_EVENT_STOPPED,
		      "Should receive STOPPED event");
}

ZTEST(stepper_closed_loop, test_get_actual_position_from_encoder)
{
	int32_t position;
	int ret;

	/*
	 * Set encoder to 90 degrees.
	 * 90 degrees = 90 * 3600 / 360 = 900 encoder counts
	 * 900 enc counts * 200 / 3600 = 50 microsteps
	 */
	fake_encoder_set_rotation_degrees(enc_dev, 90);

	ret = stepper_ctrl_get_actual_position(cl_dev, &position);
	zassert_equal(ret, 0, "get_actual_position should succeed");
	zassert_equal(position, 50, "Position should be 50 microsteps, got %d", position);
}

ZTEST(stepper_closed_loop, test_is_moving_reflects_loop_state)
{
	bool is_moving;
	int ret;

	/* Initially not moving */
	ret = stepper_ctrl_is_moving(cl_dev, &is_moving);
	zassert_equal(ret, 0, "is_moving should succeed");
	zassert_false(is_moving, "Should not be moving initially");
}

ZTEST(stepper_closed_loop, test_set_reference_position)
{
	int ret;
	int32_t position;

	/* Set encoder to 90 degrees */
	fake_encoder_set_rotation_degrees(enc_dev, 90);

	/* Set reference position to 0 at current encoder position */
	ret = stepper_ctrl_set_reference_position(cl_dev, 0);
	zassert_equal(ret, 0, "set_reference_position should succeed");

	/* Now reading position should return 0 (since encoder is at the reference) */
	ret = stepper_ctrl_get_actual_position(cl_dev, &position);
	zassert_equal(ret, 0, "get_actual_position should succeed");
	zassert_equal(position, 0, "Position should be 0 after setting reference, got %d",
		      position);
}

ZTEST(stepper_closed_loop, test_move_by_delegates_to_inner)
{
	int ret;

	/* Set encoder to current position = 0 degrees */
	fake_encoder_set_rotation_degrees(enc_dev, 0);

	ret = stepper_ctrl_move_by(cl_dev, 50);
	zassert_equal(ret, 0, "move_by should return 0, got %d", ret);

	/* Verify inner controller received the move_by call */
	zassert_equal(fake_stepper_ctrl_move_by_fake.call_count, 1,
		      "Inner move_by should be called once for initial move");
	zassert_equal(fake_stepper_ctrl_move_by_fake.arg1_val, 50,
		      "Inner move_by should receive 50 microsteps");

	/* Clean up - stop the loop */
	stepper_ctrl_stop(cl_dev);
	k_msleep(10);
}

ZTEST(stepper_closed_loop, test_configure_ramp_delegates)
{
	struct stepper_ctrl_ramp ramp = {
		.acceleration_max = 1000,
		.speed_max = 500,
		.deceleration_max = 1000,
	};

	int ret = stepper_ctrl_configure_ramp(cl_dev, &ramp);

	/* Should delegate to inner controller */
	zassert_equal(fake_stepper_ctrl_configure_ramp_fake.call_count, 1,
		      "configure_ramp should be delegated");
	zassert_equal(ret, 0, "configure_ramp should succeed");
}

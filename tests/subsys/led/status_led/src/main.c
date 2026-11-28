/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led/led_fake.h>

#include <zephyr/fff.h>
DEFINE_FFF_GLOBALS;

#include <zephyr/led/status_led.h>

#define TEST_STATUS_1 1
#define TEST_STATUS_2 65
#define TEST_STATUS_3 -10

#define TEST_PRIO_1 1
#define TEST_PRIO_2 6
#define TEST_PRIO_3 2

#define TEST_STATUS_UNKNOWN 42

#define FAKE_LED_DEV DEVICE_DT_GET(DT_NODELABEL(led_1))

static const struct status_led_anim anims[] = {
	STATUS_LED_BLINK_ANIM(TEST_STATUS_1, TEST_PRIO_1, 500, 500),
	STATUS_LED_BLINK_ANIM(TEST_STATUS_2, TEST_PRIO_2, 1000, 1000),
	STATUS_LED_ANIM(TEST_STATUS_3, TEST_PRIO_3, 2, 2000, 500),
};
STATUS_LED_DEFINE(test_status_led, FAKE_LED_DEV, 0, false, anims);

ZTEST(status_led, test_status_led_set_invalid_vals)
{
	int ret;

	ret = status_led_set(NULL, TEST_STATUS_1);
	zassert_equal(ret, -EINVAL);

	ret = status_led_set(&test_status_led, TEST_STATUS_UNKNOWN);
	zassert_equal(ret, -ENOENT);
	zassert_equal(fake_led_on_fake.call_count, 0);
}

ZTEST(status_led, test_status_led_clear_invalid_vals)
{
	int ret;

	ret = status_led_clear(NULL, TEST_STATUS_1);
	zassert_equal(ret, -EINVAL);

	ret = status_led_clear(&test_status_led, TEST_STATUS_UNKNOWN);
	zassert_equal(ret, -ENOENT);
}

ZTEST(status_led, test_status_led_set)
{
	int ret;

	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(1));
	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED_DEV);

	k_sleep(K_MSEC(500));
	zassert_equal_ptr(fake_led_off_fake.arg0_val, FAKE_LED_DEV);

	k_sleep(K_MSEC(500));
	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED_DEV);

	status_led_clear(&test_status_led, TEST_STATUS_1);
}

ZTEST(status_led, test_status_led_set_with_higher_prio)
{
	int ret;

	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(1));
	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED_DEV);

	k_sleep(K_MSEC(500));
	zassert_equal_ptr(fake_led_off_fake.arg0_val, FAKE_LED_DEV);

	k_sleep(K_MSEC(500));
	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED_DEV);

	ret = status_led_set(&test_status_led, TEST_STATUS_2);
	zassert_equal_ptr(fake_led_on_fake.arg0_val, FAKE_LED_DEV);

	k_sleep(K_MSEC(1100));
	zassert_equal_ptr(fake_led_off_fake.arg0_val, FAKE_LED_DEV);

	status_led_clear(&test_status_led, TEST_STATUS_1);
	status_led_clear(&test_status_led, TEST_STATUS_2);
}

ZTEST(status_led, test_status_led_set_with_finished_anim)
{
	int ret;

	ret = status_led_set(&test_status_led, TEST_STATUS_3);
	zassert_equal(ret, 0);

	/*
	 * Two off frames, then the default state is applied by the completion,
	 * and again once no status is active.
	 */
	k_sleep(K_MSEC(5000));
	zassert_equal(fake_led_on_fake.call_count, 2);
	zassert_equal(fake_led_off_fake.call_count, 4);

	/* Check that anim has stopped */
	k_sleep(K_MSEC(5000));
	zassert_equal(fake_led_on_fake.call_count, 2);
	zassert_equal(fake_led_off_fake.call_count, 4);
}

ZTEST(status_led, test_status_led_set_with_finished_and_higher_prio_anim)
{
	int ret;

	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(600));
	zassert_equal(fake_led_on_fake.call_count, 1);
	zassert_equal(fake_led_off_fake.call_count, 1);


	ret = status_led_set(&test_status_led, TEST_STATUS_3);
	zassert_equal(ret, 0);

	/*
	 * Check halfway through the last frame of TEST_STATUS_3, so that the
	 * tick k_sleep() adds doesn't change which frames have been played.
	 */
	k_sleep(K_MSEC(4800));
	zassert_equal(fake_led_on_fake.call_count, 3);
	zassert_equal(fake_led_off_fake.call_count, 3);

	/*
	 * Previous anim should run, after the completion of TEST_STATUS_3 has
	 * applied the default state.
	 */
	k_sleep(K_MSEC(1000));
	zassert_equal(fake_led_on_fake.call_count, 4);
	zassert_equal(fake_led_off_fake.call_count, 5);

	status_led_clear(&test_status_led, TEST_STATUS_1);
}

/*
 * Check that a status can be set again after being cleared: once no status
 * is active anymore, setting the same status again must restart its
 * animation.
 */
ZTEST(status_led, test_status_led_set_clear_set_same_status)
{
	int ret;

	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(600));
	zassert_equal(fake_led_on_fake.call_count, 1);
	zassert_equal(fake_led_off_fake.call_count, 1);

	/* No status active anymore, the animation is stopped */
	ret = status_led_clear(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	/* Setting the same status again must restart the animation */
	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	/* Clearing the last status applied the default state */
	k_sleep(K_MSEC(600));
	zassert_equal(fake_led_on_fake.call_count, 2);
	zassert_equal(fake_led_off_fake.call_count, 3);

	status_led_clear(&test_status_led, TEST_STATUS_1);
}

ZTEST(status_led, test_status_led_set_already_active)
{
	int ret;

	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(600));
	zassert_equal(fake_led_on_fake.call_count, 1);
	zassert_equal(fake_led_off_fake.call_count, 1);

	/* The animation must not be restarted */
	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);
	zassert_equal(fake_led_on_fake.call_count, 1);
}

ZTEST(status_led, test_status_led_clear_last_status)
{
	int count_on, count_off;
	int ret;

	ret = status_led_set(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	k_sleep(K_MSEC(600));

	ret = status_led_clear(&test_status_led, TEST_STATUS_1);
	zassert_equal(ret, 0);

	/* No status active anymore: the animation must be stopped */
	count_on = fake_led_on_fake.call_count;
	count_off = fake_led_off_fake.call_count;
	k_sleep(K_MSEC(2000));
	zassert_equal(fake_led_on_fake.call_count, count_on);
	zassert_equal(fake_led_off_fake.call_count, count_off);
}

ZTEST(status_led, test_status_led_finite_status_preempted_then_completes)
{
	int count_on, count_off;
	int ret;

	ret = status_led_set(&test_status_led, TEST_STATUS_3);
	zassert_equal(ret, 0);

	/* TEST_STATUS_2 has a higher priority and preempts TEST_STATUS_3 */
	ret = status_led_set(&test_status_led, TEST_STATUS_2);
	zassert_equal(ret, 0);
	k_sleep(K_MSEC(100));

	/* TEST_STATUS_3 is still active: its animation is played again */
	ret = status_led_clear(&test_status_led, TEST_STATUS_2);
	zassert_equal(ret, 0);
	count_on = fake_led_on_fake.call_count;
	count_off = fake_led_off_fake.call_count;

	/* 2 loops of 2000 ms on, 500 ms off: completes after 5 s */
	k_sleep(K_MSEC(6000));
	/*
	 * Two off frames, then the default state is applied by the completion,
	 * and again once no status is active.
	 */
	zassert_equal(fake_led_on_fake.call_count, count_on + 1);
	zassert_equal(fake_led_off_fake.call_count, count_off + 4);

	/* TEST_STATUS_3 cleared itself: the LED is not animated anymore */
	k_sleep(K_MSEC(3000));
	zassert_equal(fake_led_on_fake.call_count, count_on + 1);
	zassert_equal(fake_led_off_fake.call_count, count_off + 4);

	/* and setting it again plays its animation */
	ret = status_led_set(&test_status_led, TEST_STATUS_3);
	zassert_equal(ret, 0);
	zassert_equal(fake_led_on_fake.call_count, count_on + 2);
}

/* Leave no status active for the next test, even if this one failed */
static void status_led_after(void *fixture)
{
	ARG_UNUSED(fixture);

	status_led_clear(&test_status_led, TEST_STATUS_1);
	status_led_clear(&test_status_led, TEST_STATUS_2);
	status_led_clear(&test_status_led, TEST_STATUS_3);
}

ZTEST_SUITE(status_led, NULL, NULL, NULL, status_led_after, NULL);

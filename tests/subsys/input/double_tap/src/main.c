/*
 * Copyright 2024 Kelly Helmut Lord
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct device *const fake_dev = DEVICE_DT_GET(
		DT_NODELABEL(fake_input_device));
static const struct device *const double_tap_dev = DEVICE_DT_GET(
		DT_NODELABEL(double_tap));

DEVICE_DT_DEFINE(DT_INST(0, vnd_input_device), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static int event_count;
static struct input_event last_events[2];

static void test_cb(struct input_event *evt, void *user_data)
{
	TC_PRINT("%s: %d %x %d\n", __func__, event_count, evt->code, evt->value);

	event_count++;
	memcpy(&last_events[1], &last_events[0], sizeof(struct input_event));
	memcpy(&last_events[0], evt, sizeof(struct input_event));
}
INPUT_CALLBACK_DEFINE(double_tap_dev, test_cb, NULL);

ZTEST(double_tap, test_double_tap_test)
{
	zassert_equal(event_count, 0);

	/* ignored */
	input_report_key(fake_dev, INPUT_KEY_3, 1, true, K_FOREVER);
	input_report_key(fake_dev, INPUT_KEY_3, 0, true, K_FOREVER);
	zassert_equal(event_count, 0);
	input_report_abs(fake_dev, INPUT_KEY_0, 1, true, K_FOREVER);
	input_report_abs(fake_dev, INPUT_KEY_0, 0, true, K_FOREVER);
	zassert_equal(event_count, 0);

	/* double tap*/
	input_report_key(fake_dev, INPUT_KEY_0, 1, true, K_FOREVER);
	k_sleep(K_MSEC(50));
	input_report_key(fake_dev, INPUT_KEY_0, 0, true, K_FOREVER);
	k_sleep(K_MSEC(50));
	input_report_key(fake_dev, INPUT_KEY_0, 1, true, K_FOREVER);
	k_sleep(K_MSEC(50));
	input_report_key(fake_dev, INPUT_KEY_0, 0, true, K_FOREVER);
	zassert_equal(event_count, 2);
	zassert_equal(last_events[1].type, INPUT_EV_KEY);
	zassert_equal(last_events[1].code, INPUT_KEY_X);
	zassert_equal(last_events[1].value, 1);
	zassert_equal(last_events[0].type, INPUT_EV_KEY);
	zassert_equal(last_events[0].code, INPUT_KEY_X);
	zassert_equal(last_events[0].value, 0);

	/* double tap - other key */
	input_report_key(fake_dev, INPUT_KEY_1, 1, true, K_FOREVER);
	k_sleep(K_MSEC(50));
	input_report_key(fake_dev, INPUT_KEY_1, 0, true, K_FOREVER);
	k_sleep(K_MSEC(50));
	input_report_key(fake_dev, INPUT_KEY_1, 1, true, K_FOREVER);
	k_sleep(K_MSEC(50));
	input_report_key(fake_dev, INPUT_KEY_1, 0, true, K_FOREVER);
	zassert_equal(event_count, 4);
	zassert_equal(last_events[1].type, INPUT_EV_KEY);
	zassert_equal(last_events[1].code, INPUT_KEY_Y);
	zassert_equal(last_events[1].value, 1);
	zassert_equal(last_events[0].type, INPUT_EV_KEY);
	zassert_equal(last_events[0].code, INPUT_KEY_Y);
	zassert_equal(last_events[0].value, 0);
}

ZTEST_SUITE(double_tap, NULL, NULL, NULL, NULL, NULL);

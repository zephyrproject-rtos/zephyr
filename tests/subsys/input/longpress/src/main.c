/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct device *const fake_dev = DEVICE_DT_GET(
		DT_NODELABEL(fake_input_device));
static const struct device *const longpress_dev = DEVICE_DT_GET(
		DT_NODELABEL(longpress));
static const struct device *const longpress_no_short_dev = DEVICE_DT_GET(
		DT_NODELABEL(longpress_no_short));

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
INPUT_CALLBACK_DEFINE(longpress_dev, test_cb, NULL);

static int event_count_no_short;
static struct input_event last_events_no_short[2];
static void test_cb_no_short(struct input_event *evt, void *user_data)
{
	TC_PRINT("%s: %d %x %d\n", __func__, event_count_no_short, evt->code, evt->value);

	event_count_no_short++;
	memcpy(&last_events_no_short[1], &last_events_no_short[0], sizeof(struct input_event));
	memcpy(&last_events_no_short[0], evt, sizeof(struct input_event));
}
INPUT_CALLBACK_DEFINE(longpress_no_short_dev, test_cb_no_short, NULL);

ZTEST(longpress, test_longpress_test)
{
	zassert_equal(event_count, 0);

	/* ignored */
	input_report_key(fake_dev, INPUT_KEY_3, 1, true, K_FOREVER);
	input_report_key(fake_dev, INPUT_KEY_3, 0, true, K_FOREVER);
	zassert_equal(event_count, 0);
	zassert_equal(event_count_no_short, 0);
	input_report_abs(fake_dev, INPUT_KEY_0, 1, true, K_FOREVER);
	input_report_abs(fake_dev, INPUT_KEY_0, 0, true, K_FOREVER);
	zassert_equal(event_count, 0);
	zassert_equal(event_count_no_short, 0);

	/* short press */
	input_report_key(fake_dev, INPUT_KEY_0, 1, true, K_FOREVER);
	k_sleep(K_MSEC(50));
	input_report_key(fake_dev, INPUT_KEY_0, 0, true, K_FOREVER);
	zassert_equal(event_count, 2);
	zassert_equal(last_events[1].type, INPUT_EV_KEY);
	zassert_equal(last_events[1].code, INPUT_KEY_A);
	zassert_equal(last_events[1].value, 1);
	zassert_equal(last_events[0].type, INPUT_EV_KEY);
	zassert_equal(last_events[0].code, INPUT_KEY_A);
	zassert_equal(last_events[0].value, 0);
	zassert_equal(event_count_no_short, 0);

	/* short press - other key */
	input_report_key(fake_dev, INPUT_KEY_1, 1, true, K_FOREVER);
	k_sleep(K_MSEC(50));
	input_report_key(fake_dev, INPUT_KEY_1, 0, true, K_FOREVER);
	zassert_equal(event_count, 4);
	zassert_equal(last_events[1].type, INPUT_EV_KEY);
	zassert_equal(last_events[1].code, INPUT_KEY_B);
	zassert_equal(last_events[1].value, 1);
	zassert_equal(last_events[0].type, INPUT_EV_KEY);
	zassert_equal(last_events[0].code, INPUT_KEY_B);
	zassert_equal(last_events[0].value, 0);
	zassert_equal(event_count_no_short, 0);

	/* long press */
	input_report_key(fake_dev, INPUT_KEY_0, 1, true, K_FOREVER);
	k_sleep(K_MSEC(150));
	input_report_key(fake_dev, INPUT_KEY_0, 0, true, K_FOREVER);
	zassert_equal(event_count, 6);
	zassert_equal(last_events[1].type, INPUT_EV_KEY);
	zassert_equal(last_events[1].code, INPUT_KEY_X);
	zassert_equal(last_events[1].value, 1);
	zassert_equal(last_events[0].type, INPUT_EV_KEY);
	zassert_equal(last_events[0].code, INPUT_KEY_X);
	zassert_equal(last_events[0].value, 0);

	zassert_equal(event_count_no_short, 2);
	zassert_equal(last_events_no_short[1].type, INPUT_EV_KEY);
	zassert_equal(last_events_no_short[1].code, INPUT_KEY_X);
	zassert_equal(last_events_no_short[1].value, 1);
	zassert_equal(last_events_no_short[0].type, INPUT_EV_KEY);
	zassert_equal(last_events_no_short[0].code, INPUT_KEY_X);
	zassert_equal(last_events_no_short[0].value, 0);

	/* long press - other key */
	input_report_key(fake_dev, INPUT_KEY_1, 1, true, K_FOREVER);
	k_sleep(K_MSEC(150));
	input_report_key(fake_dev, INPUT_KEY_1, 0, true, K_FOREVER);
	zassert_equal(event_count, 8);
	zassert_equal(last_events[1].type, INPUT_EV_KEY);
	zassert_equal(last_events[1].code, INPUT_KEY_Y);
	zassert_equal(last_events[1].value, 1);
	zassert_equal(last_events[0].type, INPUT_EV_KEY);
	zassert_equal(last_events[0].code, INPUT_KEY_Y);
	zassert_equal(last_events[0].value, 0);

	zassert_equal(event_count_no_short, 4);
	zassert_equal(last_events_no_short[1].type, INPUT_EV_KEY);
	zassert_equal(last_events_no_short[1].code, INPUT_KEY_Y);
	zassert_equal(last_events_no_short[1].value, 1);
	zassert_equal(last_events_no_short[0].type, INPUT_EV_KEY);
	zassert_equal(last_events_no_short[0].code, INPUT_KEY_Y);
	zassert_equal(last_events_no_short[0].value, 0);
}

ZTEST_SUITE(longpress, NULL, NULL, NULL, NULL, NULL);

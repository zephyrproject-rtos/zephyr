/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct device *kscan_dev = DEVICE_DT_GET(
		DT_NODELABEL(kscan_input));
static const struct device *input_dev = DEVICE_DT_GET(
		DT_NODELABEL(fake_input_device));

DEVICE_DT_DEFINE(DT_INST(0, vnd_input_device), NULL, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static struct {
	uint32_t row;
	uint32_t col;
	bool pressed;
} last_cb_val;
static int callback_calls_count;

static void kscan_callback(const struct device *dev, uint32_t row, uint32_t col,
			   bool pressed)
{
	TC_PRINT("row = %u col = %u %d\n", row, col, pressed);

	callback_calls_count++;
	last_cb_val.row = row;
	last_cb_val.col = col;
	last_cb_val.pressed = pressed;
}

ZTEST(kscan_input, test_kscan_input)
{
	kscan_config(kscan_dev, kscan_callback);
	kscan_enable_callback(kscan_dev);

	input_report_abs(input_dev, INPUT_ABS_X, 101, false, K_FOREVER);
	zassert_equal(callback_calls_count, 0);

	input_report_abs(input_dev, INPUT_ABS_Y, 102, false, K_FOREVER);
	zassert_equal(callback_calls_count, 0);

	input_report_key(input_dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	zassert_equal(callback_calls_count, 1);
	zassert_equal(last_cb_val.col, 101);
	zassert_equal(last_cb_val.row, 102);
	zassert_equal(last_cb_val.pressed, true);

	input_report_abs(input_dev, INPUT_ABS_X, 103, true, K_FOREVER);
	zassert_equal(callback_calls_count, 2);
	zassert_equal(last_cb_val.col, 103);
	zassert_equal(last_cb_val.row, 102);
	zassert_equal(last_cb_val.pressed, true);

	input_report_key(input_dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	zassert_equal(callback_calls_count, 3);
	zassert_equal(last_cb_val.col, 103);
	zassert_equal(last_cb_val.row, 102);
	zassert_equal(last_cb_val.pressed, false);
}

ZTEST_SUITE(kscan_input, NULL, NULL, NULL, NULL, NULL);

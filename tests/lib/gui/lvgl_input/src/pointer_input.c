/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/input/input.h>
#include <lvgl_input_device.h>
#include <lvgl_display.h>

#include <lvgl.h>

#define LVGL_POINTER DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_pointer_input)

#define DISPLAY_NODE   DT_CHOSEN(zephyr_display)
#define DISPLAY_WIDTH  DT_PROP(DISPLAY_NODE, width)
#define DISPLAY_HEIGHT DT_PROP(DISPLAY_NODE, height)

static void emit_pointer_events(lv_indev_t *indev, lv_point_t *point, bool pressed)
{
	if (pressed) {
		input_report_abs(NULL, INPUT_ABS_X, point->x, false, K_FOREVER);
		input_report_abs(NULL, INPUT_ABS_Y, point->y, false, K_FOREVER);
		input_report_key(NULL, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	} else {
		input_report_key(NULL, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	}

	/* Force LVGL to read the event */
	lv_indev_read_timer_cb(indev->driver->read_timer);
}

static void rotate_and_check(enum display_orientation orientation, lv_point_t *pressed,
			     lv_point_t *expected)
{
	int ret;
	const struct device *display_dev = DEVICE_DT_GET(DISPLAY_NODE);
	const struct device *pointer = DEVICE_DT_GET(LVGL_POINTER);
	lv_indev_t *pointer_indev = lvgl_input_get_indev(pointer);
	lv_point_t press_reported = {0};
	lv_point_t release_reported = {0};

	zassert_not_null(pointer_indev, "No underlying indev for pointer");

	ret = display_set_orientation(display_dev, orientation);
	zassert_ok(ret, "Setting display orientation failed");

	ret = lvgl_reload_display_capabilities();
	zassert_ok(ret, "Reloading display capabilities for LVGL failed");

	/* Simulate press event */
	emit_pointer_events(pointer_indev, pressed, true);
	lv_indev_get_point(pointer_indev, &press_reported);

	/* Simulate release event */
	emit_pointer_events(pointer_indev, pressed, false);
	lv_indev_get_point(pointer_indev, &release_reported);

	printk("\tExpected: x:%u y:%u\n", expected->x, expected->y);
	printk("\tPress:    x:%u y:%u\n", press_reported.x, press_reported.y);
	printk("\tRelease:  x:%u y:%u\n", release_reported.x, release_reported.y);

	zassert_equal(expected->x, press_reported.x, "Press: X coordinates do not match");
	zassert_equal(expected->y, press_reported.y, "Press: Y coordinates do not match");

	zassert_equal(expected->x, release_reported.x, "Release: X coordinates do not match");
	zassert_equal(expected->y, release_reported.y, "Release: Y coordinates do not match");
}

ZTEST(lvgl_input_pointer, test_no_rotation)
{
	lv_point_t pressed = {.x = 20, .y = 150};

	rotate_and_check(DISPLAY_ORIENTATION_NORMAL, &pressed, &pressed);
}

ZTEST(lvgl_input_pointer, test_rotation_90)
{
	lv_point_t pressed = {.x = 20, .y = 150};
	lv_point_t expected = {.x = DISPLAY_HEIGHT - pressed.y, .y = pressed.x};

	rotate_and_check(DISPLAY_ORIENTATION_ROTATED_90, &pressed, &expected);
}

ZTEST(lvgl_input_pointer, test_rotation_180)
{
	lv_point_t pressed = {.x = 20, .y = 150};
	lv_point_t expected = {.x = DISPLAY_WIDTH - pressed.x, .y = DISPLAY_HEIGHT - pressed.y};

	rotate_and_check(DISPLAY_ORIENTATION_ROTATED_180, &pressed, &expected);
}

ZTEST(lvgl_input_pointer, test_rotation_270)
{
	lv_point_t pressed = {.x = 20, .y = 150};
	lv_point_t expected = {.x = pressed.y, .y = DISPLAY_WIDTH - pressed.x};

	rotate_and_check(DISPLAY_ORIENTATION_ROTATED_270, &pressed, &expected);
}

ZTEST_SUITE(lvgl_input_pointer, NULL, NULL, NULL, NULL, NULL);

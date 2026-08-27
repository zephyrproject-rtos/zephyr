/*
 * Copyright (c) 2026 Arkadiusz Grzelka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT test_touchscreen_common

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_touch.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* Minimal touchscreen driver: the common config is the whole config. */

struct touch_test_config {
	struct input_touchscreen_common_config common;
};

#define TOUCH_TEST_INIT(inst)                                                          \
	static const struct touch_test_config touch_test_config_##inst = {               \
		.common = INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(inst),                  \
	};                                                                                 \
	INPUT_TOUCH_STRUCT_CHECK(struct touch_test_config)                               \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL,                                    \
			       &touch_test_config_##inst, POST_KERNEL,                   \
			       CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(TOUCH_TEST_INIT)

static const struct device *const dev_passthrough =
	DEVICE_DT_GET(DT_NODELABEL(test_ts_passthrough));
static const struct device *const dev_inverted =
	DEVICE_DT_GET(DT_NODELABEL(test_ts_inverted));
static const struct device *const dev_swapped =
	DEVICE_DT_GET(DT_NODELABEL(test_ts_swapped));
static const struct device *const dev_scaled =
	DEVICE_DT_GET(DT_NODELABEL(test_ts_scaled));
static const struct device *const dev_scaled_swapped =
	DEVICE_DT_GET(DT_NODELABEL(test_ts_scaled_swapped));
static const struct device *const dev_scale_disabled =
	DEVICE_DT_GET(DT_NODELABEL(test_ts_scale_disabled));
static const struct device *const dev_scaled_from_display =
	DEVICE_DT_GET(DT_NODELABEL(test_ts_from_display));

/* Capture support: input_touchscreen_report_pos always emits exactly one
 * INPUT_ABS_X and one INPUT_ABS_Y event, in that order. Capture both and
 * check them together.
 */

static int event_count;
static struct input_event captured[2];

static void test_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (event_count < 2) {
		captured[event_count] = *evt;
	}
	event_count++;
}
INPUT_CALLBACK_DEFINE(NULL, test_cb, NULL);

static void reset_capture(void)
{
	event_count = 0;
	memset(captured, 0, sizeof(captured));
}

/* Fetch the captured value reported under a given code, regardless of order. */
static uint32_t captured_value(uint32_t code)
{
	zassert_equal(event_count, 2, "expected exactly 2 events, got %d", event_count);
	for (int i = 0; i < 2; i++) {
		if (captured[i].code == code) {
			return captured[i].value;
		}
	}
	zassert_unreachable("code %d not reported", code);
	return 0;
}

static void expect_pos(const struct device *dev, uint32_t x, uint32_t y,
			uint32_t expect_abs_x, uint32_t expect_abs_y)
{
	reset_capture();
	input_touchscreen_report_pos(dev, x, y, K_FOREVER);
	zassert_equal(captured_value(INPUT_ABS_X), expect_abs_x);
	zassert_equal(captured_value(INPUT_ABS_Y), expect_abs_y);
}

/* 1. Pass-through: no orientation/scaling properties set. */
ZTEST(touchscreen_common, test_passthrough)
{
	expect_pos(dev_passthrough, 0, 0, 0, 0);
	expect_pos(dev_passthrough, 12, 34, 12, 34);
}

/* 2. inverted-x / inverted-y: value becomes screen_{width,height} - value. */
ZTEST(touchscreen_common, test_inverted)
{
	/* screen-width = 100, screen-height = 50 */
	expect_pos(dev_inverted, 0, 0, 100, 50);
	expect_pos(dev_inverted, 10, 20, 90, 30);
	expect_pos(dev_inverted, 100, 50, 0, 0);
}

/* 3. swapped-x-y: x is reported under INPUT_ABS_Y and y under INPUT_ABS_X. */
ZTEST(touchscreen_common, test_swapped)
{
	expect_pos(dev_swapped, 5, 9, /* expect_abs_x = */ 9, /* expect_abs_y = */ 5);
	expect_pos(dev_swapped, 0, 1, 1, 0);
}

/* 4. output-width/height set, no swap: each axis scaled by its own ratio. */
ZTEST(touchscreen_common, test_scaled_no_swap)
{
	/* screen 480x272 -> display 240x136, i.e. half scale on both axes. */
	expect_pos(dev_scaled, 0, 0, 0, 0);
	expect_pos(dev_scaled, 479, 271, 479 * 240 / 480, 271 * 136 / 272);
	expect_pos(dev_scaled, 240, 136, 240 * 240 / 480, 136 * 136 / 272);
}

/*
 * 5. output-width/height set together with swapped-x-y. This is the whole
 * reason the change exists: swapping alone only works when the touchscreen
 * and the display share an aspect ratio. On a rotated non-square panel the
 * swapped axes still carry the *other* axis' source range, so each value
 * must be rescaled using the destination dimension its swapped code reports
 * under, divided by its own source range.
 *
 * Controller and display are both 480x272 here (screen-width/height ==
 * output-width/height), so this is not a no-op: because of the swap, X
 * (source range 480) ends up reported under INPUT_ABS_Y and must be scaled
 * against output-height (272), and Y (source range 272) ends up reported
 * under INPUT_ABS_X and must be scaled against output-width (480).
 */
ZTEST(touchscreen_common, test_scaled_and_swapped)
{
	/* Corners. */
	expect_pos(dev_scaled_swapped, 0, 0, /* abs_x = */ 0, /* abs_y = */ 0);
	/* controller x=479 -> INPUT_ABS_Y = 479 * 272 / 480 = 271 */
	expect_pos(dev_scaled_swapped, 479, 0, /* abs_x = */ 0, /* abs_y = */ 271);
	/* controller y=271 -> INPUT_ABS_X = 271 * 480 / 272 = 478 */
	expect_pos(dev_scaled_swapped, 0, 271, /* abs_x = */ 478, /* abs_y = */ 0);
	expect_pos(dev_scaled_swapped, 479, 271, /* abs_x = */ 478, /* abs_y = */ 271);

	/* Midpoint: at exactly half scale in both directions round-trips cleanly. */
	expect_pos(dev_scaled_swapped, 240, 136, /* abs_x = */ 240, /* abs_y = */ 136);
}

/* 6. Scaling stays disabled unless both output-width and output-height are set. */
ZTEST(touchscreen_common, test_scale_disabled_when_partial)
{
	/* Only output-width is set on this node; coordinates pass through. */
	expect_pos(dev_scale_disabled, 479, 271, 479, 271);
}

/*
 * 7. Output dimensions sourced from a `display` phandle instead of explicit
 * output-width/height. The referenced display node is 240x136, so this behaves
 * exactly like case 4 (half scale on both axes) without duplicating the panel
 * resolution on the touch node.
 */
ZTEST(touchscreen_common, test_scaled_from_display_phandle)
{
	/* screen 480x272 -> display 240x136 via phandle, i.e. half scale. */
	expect_pos(dev_scaled_from_display, 0, 0, 0, 0);
	expect_pos(dev_scaled_from_display, 479, 271, 479 * 240 / 480, 271 * 136 / 272);
	expect_pos(dev_scaled_from_display, 240, 136, 240 * 240 / 480, 136 * 136 / 272);
}

ZTEST_SUITE(touchscreen_common, NULL, NULL, NULL, NULL, NULL);

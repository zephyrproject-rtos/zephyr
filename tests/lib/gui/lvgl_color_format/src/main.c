/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/ztest.h>

#include <lvgl.h>
#include <lvgl_zephyr.h>

static const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

/*
 * An RGB_565X panel renders in LVGL's native LV_COLOR_FORMAT_RGB565_SWAPPED
 * format. Combining it with the legacy CONFIG_LV_COLOR_16_SWAP byte-swaps the
 * frame buffer a second time, so lvgl_init() must reject that combination.
 * Without the swap it must succeed and select the swapped format. The two
 * build scenarios pin both sides of that boundary.
 */
ZTEST(lvgl_color_format, test_rgb565x_swap_policy)
{
	int ret;

	zassert_true(device_is_ready(display_dev), "display device not ready");
	zassert_ok(display_set_pixel_format(display_dev, PIXEL_FORMAT_RGB_565X),
		   "dummy display should accept RGB_565X");

	ret = lvgl_init();

	if (IS_ENABLED(CONFIG_LV_COLOR_16_SWAP)) {
		zassert_equal(ret, -ENOTSUP,
			      "RGB_565X with CONFIG_LV_COLOR_16_SWAP must be rejected");
	} else {
		lv_display_t *display;

		zassert_ok(ret, "valid RGB_565X configuration must initialize");

		display = lv_display_get_default();
		zassert_not_null(display, "default LVGL display missing");
		zassert_equal(lv_display_get_color_format(display), LV_COLOR_FORMAT_RGB565_SWAPPED,
			      "RGB_565X must render as RGB565_SWAPPED");
	}

	lv_deinit();
}

ZTEST_SUITE(lvgl_color_format, NULL, NULL, NULL, NULL, NULL);

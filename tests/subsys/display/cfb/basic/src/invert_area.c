/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/display.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>

#include "testdata.h"
#include "utils.h"

LOG_MODULE_REGISTER(invert_area, CONFIG_DISPLAY_LOG_LEVEL);

static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const uint32_t display_width = DT_PROP(DT_CHOSEN(zephyr_display), width);
static const uint32_t display_height = DT_PROP(DT_CHOSEN(zephyr_display), height);

/**
 * Fill the buffer with 0 before running tests.
 */
static void cfb_test_before(void *text_fixture)
{
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width / 8,
	};

	memset(read_buffer, 0, sizeof(read_buffer));
	zassert_ok(display_write(dev, 0, 0, &desc, read_buffer));

	zassert_ok(display_blanking_off(dev));

	zassert_ok(cfb_framebuffer_init(dev));
}

static void cfb_test_after(void *test_fixture)
{
	cfb_framebuffer_deinit(dev);
}

ZTEST(invert_area, test_invert_area_whole_screen)
{
	zassert_ok(cfb_invert_area(dev, 0, 0, 320, 240));
	zassert_ok(cfb_framebuffer_finalize(dev), "cfb_framebuffer_finalize failed");

	zassert_true(verify_color_inside_rect(0, 0, 320, 240, 0xFFFFFF));
}

ZTEST(invert_area, test_invert_area_overlapped_2times)
{
	zassert_ok(cfb_invert_area(dev, 33, 37, 79, 77));
	zassert_ok(cfb_invert_area(dev, 100, 37, 53, 77));
	zassert_ok(cfb_framebuffer_finalize(dev), "cfb_framebuffer_finalize failed");

	zassert_true(verify_color_inside_rect(33, 37, 67, 77, 0xFFFFFF));
	zassert_true(verify_color_inside_rect(100, 37, 12, 77, 0x0));
	zassert_true(verify_color_inside_rect(112, 37, 41, 77, 0xFFFFFF));
	zassert_true(verify_color_outside_rect(33, 37, 120, 77, 0x0));
}

ZTEST(invert_area, test_invert_area_overlap_top_left)
{
	int err;

	err = cfb_invert_area(dev, -10, -10, 20, 20);
	zassert_not_ok(err, "out of rect");
}

ZTEST(invert_area, test_invert_area_overlap_top_right)
{
	int err;

	err = cfb_invert_area(dev, 230, -10, 20, 20);
	zassert_not_ok(err, "out of rect");
}

ZTEST(invert_area, test_invert_area_overlap_bottom_left)
{
	int err;

	err = cfb_invert_area(dev, -10, display_height - 10, 20, 20);
	zassert_not_ok(err, "out of rect");
}

ZTEST(invert_area, test_invert_area_overlap_bottom_right)
{
	zassert_ok(cfb_invert_area(dev, display_width - 10, display_height - 10, 20, 20));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(display_width - 10, display_height - 10, 10, 10,
					      0xFFFFFF));
	zassert_true(
		verify_color_outside_rect(display_width - 10, display_height - 10, 10, 10, 0x0));
}

ZTEST(invert_area, test_invert_area_outside_top_left)
{
	zassert_not_ok(cfb_invert_area(dev, -10, -10, 10, 10), "out of rect");
}

ZTEST(invert_area, test_invert_area_outside_bottom_right)
{
	zassert_not_ok(cfb_invert_area(dev, display_width, display_height, 20, 20), "out of rect");
}

ZTEST_SUITE(invert_area, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

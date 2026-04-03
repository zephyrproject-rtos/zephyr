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

LOG_MODULE_REGISTER(draw_rect, CONFIG_DISPLAY_LOG_LEVEL);

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

/*
 * normal rendering
 */
ZTEST(draw_rect, test_draw_rect_1123_at_0_0)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image_and_bg(0, 0, rectspace1123, 11, 23, 0), "");
}

ZTEST(draw_rect, test_draw_rect_1123_at_1_1)
{
	struct cfb_position start = {1, 1};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image_and_bg(1, 1, rectspace1123, 11, 23, 0), "");
}

/* tile border case */
ZTEST(draw_rect, test_draw_rect_1123_at_9_15)
{
	struct cfb_position start = {9, 15};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image_and_bg(9, 15, rectspace1123, 11, 23, 0), "");
}

ZTEST(draw_rect, test_draw_rect_1123_at_10_16)
{
	struct cfb_position start = {10, 16};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image_and_bg(10, 16, rectspace1123, 11, 23, 0), "");
}

ZTEST(draw_rect, test_draw_rect_1123_at_11_17)
{
	struct cfb_position start = {11, 17};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image_and_bg(11, 17, rectspace1123, 11, 23, 0), "");
}

/*
 * Case of including coordinates outside the area
 */
ZTEST(draw_rect, test_draw_rect_1123_outside_top_left)
{
	struct cfb_position start = {-(11 - 3), -(23 - 4)};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image_and_bg(0, 0, outside_top_left, 3, 4, 0), "");
}

ZTEST(draw_rect, test_draw_rect_1123_outside_top_right)
{
	struct cfb_position start = {display_width - 5, -(23 - 8)};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(display_width - 5, 0, outside_top_right, 5, 8), "");
}

ZTEST(draw_rect, test_draw_rect_1123_outside_bottom_right)
{
	struct cfb_position start = {display_width - 3, display_height - 5};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(
		verify_image(display_width - 3, display_height - 5, outside_bottom_right, 3, 5),
		"");
}

ZTEST(draw_rect, test_draw_rect_1123_outside_bottom_left)
{
	struct cfb_position start = {-(11 - 3), display_height - 14};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(dev, &start, &end), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(0, display_height - 14, outside_bottom_left, 3, 14), "");
}

ZTEST_SUITE(draw_rect, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

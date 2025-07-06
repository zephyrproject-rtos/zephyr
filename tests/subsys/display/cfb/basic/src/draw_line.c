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

LOG_MODULE_REGISTER(draw_line, CONFIG_DISPLAY_LOG_LEVEL);

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

ZTEST(draw_line, test_draw_line_top_end)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {display_width, 0};

	zassert_ok(cfb_draw_line(dev, &start, &end));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, 1, 0xFFFFFF));
}

ZTEST(draw_line, test_draw_line_left_end)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {0, display_height};

	zassert_ok(cfb_draw_line(dev, &start, &end));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, 1, display_height, 0xFFFFFF));
}

ZTEST(draw_line, test_draw_right_end)
{
	struct cfb_position start = {display_width - 1, 0};
	struct cfb_position end = {display_width - 1, display_height};

	zassert_ok(cfb_draw_line(dev, &start, &end));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(display_width - 1, 0, 1, display_height, 0xFFFFFF));
}

ZTEST(draw_line, test_draw_line_bottom_end)
{
	struct cfb_position start = {0, 239};
	struct cfb_position end = {display_width, 239};

	zassert_ok(cfb_draw_line(dev, &start, &end));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, display_height - 1, display_width, 1, 0xFFFFFF));
}

ZTEST(draw_line, test_render_twice_on_same_tile)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {display_width, 0};

	zassert_ok(cfb_draw_line(dev, &start, &end));
	start.y = 7;
	end.y = 7;
	zassert_ok(cfb_draw_line(dev, &start, &end));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, 1, 0xFFFFFF));
	zassert_true(verify_color_inside_rect(0, 1, display_width, 6, 0x0));
	zassert_true(verify_color_inside_rect(0, 7, display_width, 1, 0xFFFFFF));
	zassert_true(verify_color_inside_rect(0, 8, display_width, 232, 0x0));
}

ZTEST(draw_line, test_crossing_diagonally_end_to_end)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {320, 240};

	zassert_ok(cfb_draw_line(dev, &start, &end));
	zassert_ok(cfb_framebuffer_finalize(dev));

	for (size_t i = 0; i < 10; i++) {
		zassert_true(verify_image(32 * i, 24 * i, diagonal3224, 32, 24));
	}
}

ZTEST(draw_line, test_crossing_diagonally_from_outside_area)
{
	struct cfb_position start = {-32, -48};
	struct cfb_position end = {384, 264};

	zassert_ok(cfb_draw_line(dev, &start, &end));
	zassert_ok(cfb_framebuffer_finalize(dev));

	for (size_t i = 0; i < 9; i++) {
		zassert_true(verify_image(32 + 32 * i, 24 * i, diagonal3224, 32, 24));
	}
}

ZTEST_SUITE(draw_line, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

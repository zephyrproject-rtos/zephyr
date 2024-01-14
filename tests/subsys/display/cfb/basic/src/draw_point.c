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

LOG_MODULE_REGISTER(draw_point, CONFIG_DISPLAY_LOG_LEVEL);

static void *cfb_test_init(void)
{
	if (!display_initialized) {
		cfb_framebuffer_init(dev);
		display_initialized = true;
	}

	return NULL;
}

/**
 * Fill the buffer with 0 before running tests.
 */
static void cfb_test_before(void *text_fixture)
{
	display_get_capabilities(dev, &display_caps);

	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width / 8,
	};

	memset(read_buffer, 0, sizeof(read_buffer));
	display_write(dev, 0, 0, &desc, read_buffer);

	display_blanking_off(dev);

	cfb_framebuffer_clear(dev, false);
}

/*
 * normal rendering
 */
ZTEST(draw_point, test_draw_point_at_0_0)
{
	struct cfb_position pos = {0, 0};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(0, 0, 1, 0));
}

ZTEST(draw_point, test_draw_point_at_1_1)
{
	struct cfb_position pos = {1, 1};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(1, 1, 1, 0));
}

/*
 * around tile border
 */
ZTEST(draw_point, test_draw_pont_at_9_15)
{
	struct cfb_position pos = {9, 15};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(9, 15, 1, 0));
}

ZTEST(draw_point, test_draw_point_at_10_16)
{
	struct cfb_position pos = {10, 16};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(10, 16, 1, 0));
}

ZTEST(draw_point, test_draw_point_at_11_17)
{
	struct cfb_position pos = {11, 17};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(11, 17, 1, 0));
}

ZTEST(draw_point, test_draw_point_twice_on_same_tile)
{
	struct cfb_position pos = {10, 0};

	pos.y = 7;
	zassert_ok(cfb_draw_point(dev, &pos));

	pos.y = 8;
	zassert_ok(cfb_draw_point(dev, &pos));

	pos.y = 9;
	zassert_ok(cfb_draw_point(dev, &pos));

	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(10, 7, 1, 3, 0xFFFFFF));
	zassert_true(verify_color_outside_rect(10, 7, 1, 3, 0x0));
}

ZTEST(draw_point, test_draw_point_outside_top_left)
{
	struct cfb_position pos = {0, -1};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_point, test_draw_point_outside_top_right)
{
	struct cfb_position pos = {display_width, 0};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_point, test_draw_point_outside_bottom_right)
{
	struct cfb_position pos = {0, display_height};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_point, test_draw_point_outside_bottom_left)
{
	struct cfb_position pos = {-1, display_height};

	zassert_ok(cfb_draw_point(dev, &pos));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST_SUITE(draw_point, NULL, cfb_test_init, cfb_test_before, NULL, NULL);

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

static struct cfb_display *disp;
static struct cfb_framebuffer *fb;

/**
 * Fill the buffer with 0 before running tests.
 */
static void cfb_test_before(void *text_fixture)
{
	disp = display_init();
	fb = cfb_display_get_framebuffer(disp);
}

static void cfb_test_after(void *test_fixture)
{
	display_deinit(disp);
}

/*
 * normal rendering
 */
ZTEST(draw_point, test_draw_point_at_0_0)
{
	struct cfb_position pos = {0, 0};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_pixel_and_bg(0, 0, 1, 0));
}

ZTEST(draw_point, test_draw_point_at_1_1)
{
	struct cfb_position pos = {1, 1};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_pixel_and_bg(1, 1, 1, 0));
}

/*
 * around tile border
 */
ZTEST(draw_point, test_draw_pont_at_9_15)
{
	struct cfb_position pos = {9, 15};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_pixel_and_bg(9, 15, 1, 0));
}

ZTEST(draw_point, test_draw_point_at_10_16)
{
	struct cfb_position pos = {10, 16};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_pixel_and_bg(10, 16, 1, 0));
}

ZTEST(draw_point, test_draw_point_at_11_17)
{
	struct cfb_position pos = {11, 17};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_pixel_and_bg(11, 17, 1, 0));
}

ZTEST(draw_point, test_draw_point_twice_on_same_tile)
{
	struct cfb_position pos = {10, 0};

	pos.y = 7;
	zassert_ok(cfb_draw_point(fb, &pos));

	pos.y = 8;
	zassert_ok(cfb_draw_point(fb, &pos));

	pos.y = 9;
	zassert_ok(cfb_draw_point(fb, &pos));

	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(10, 7, 1, 3, 0xFFFFFF));
	zassert_true(verify_color_outside_rect(10, 7, 1, 3, 0x0));
}

ZTEST(draw_point, test_draw_point_outside_top_left)
{
	struct cfb_position pos = {0, -1};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_point, test_draw_point_outside_top_right)
{
	struct cfb_position pos = {display_width, 0};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_point, test_draw_point_outside_bottom_right)
{
	struct cfb_position pos = {0, display_height};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_point, test_draw_point_outside_bottom_left)
{
	struct cfb_position pos = {-1, display_height};

	zassert_ok(cfb_draw_point(fb, &pos));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST_SUITE(draw_point, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

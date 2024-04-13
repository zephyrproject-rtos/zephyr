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
ZTEST(draw_rect, test_draw_rect_1123_at_0_0)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(verify_image_and_bg(0, 0, rectspace1123, 11, 23, COLOR_BLACK), "");
}

ZTEST(draw_rect, test_draw_rect_1123_at_1_1)
{
	struct cfb_position start = {1, 1};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(verify_image_and_bg(1, 1, rectspace1123, 11, 23, COLOR_BLACK), "");
}

/* tile border case */
ZTEST(draw_rect, test_draw_rect_1123_at_9_15)
{
	struct cfb_position start = {9, 15};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(verify_image_and_bg(9, 15, rectspace1123, 11, 23, COLOR_BLACK), "");
}

ZTEST(draw_rect, test_draw_rect_1123_at_10_16)
{
	struct cfb_position start = {10, 16};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(verify_image_and_bg(10, 16, rectspace1123, 11, 23, COLOR_BLACK), "");
}

ZTEST(draw_rect, test_draw_rect_1123_at_11_17)
{
	struct cfb_position start = {11, 17};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(verify_image_and_bg(11, 17, rectspace1123, 11, 23, COLOR_BLACK), "");
}

/*
 * Case of including coordinates outside the area
 */
ZTEST(draw_rect, test_draw_rect_1123_outside_top_left)
{
	struct cfb_position start = {-(11 - 3), -(23 - 4)};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(verify_image_and_bg(0, 0, outside_top_left, 3, 4, COLOR_BLACK), "");
}

ZTEST(draw_rect, test_draw_rect_1123_outside_top_right)
{
	struct cfb_position start = {display_width - 5, -(23 - 8)};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(verify_image(display_width - 5, 0, outside_top_right, 5, 8), "");
}

ZTEST(draw_rect, test_draw_rect_1123_outside_bottom_right)
{
	struct cfb_position start = {display_width - 3, display_height - 5};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(
		verify_image(display_width - 3, display_height - 5, outside_bottom_right, 3, 5),
		"");
}

ZTEST(draw_rect, test_draw_rect_1123_outside_bottom_left)
{
	struct cfb_position start = {-(11 - 3), display_height - 14};
	struct cfb_position end = {start.x + 10, start.y + 22};

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb), "");

	zassert_true(verify_image(0, display_height - 14, outside_bottom_left, 3, 14), "");
}

ZTEST(draw_rect, test_draw_rect_at_0_0_red)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {start.x + 10, start.y + 22};

	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0xFF, 0, 0, 0));

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(0, 0, 11, 23, COLOR_BLACK), "");
	zassert_true(verify_color_inside_rect(1, 1, 9, 21, COLOR_BLACK), "");

	zassert_true(verify_color_inside_rect(0, 0, 10, 1, COLOR_RED));
	zassert_true(verify_color_inside_rect(0, 0, 1, 22, COLOR_RED));
	zassert_true(verify_color_inside_rect(10, 0, 1, 22, COLOR_RED));
	zassert_true(verify_color_inside_rect(0, 22, 10, 1, COLOR_RED));
}

ZTEST(draw_rect, test_draw_rect_at_0_0_green)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {start.x + 10, start.y + 22};

	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0, 0xFF, 0, 0));

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(0, 0, 11, 23, COLOR_BLACK), "");
	zassert_true(verify_color_inside_rect(1, 1, 9, 21, COLOR_BLACK), "");

	zassert_true(verify_color_inside_rect(0, 0, 10, 1, COLOR_GREEN));
	zassert_true(verify_color_inside_rect(0, 0, 1, 22, COLOR_GREEN));
	zassert_true(verify_color_inside_rect(10, 0, 1, 22, COLOR_GREEN));
	zassert_true(verify_color_inside_rect(0, 22, 10, 1, COLOR_GREEN));
}

ZTEST(draw_rect, test_draw_rect_at_0_0_blue)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {start.x + 10, start.y + 22};

	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0, 0, 0xFF, 0));

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(0, 0, 11, 23, COLOR_BLACK), "");
	zassert_true(verify_color_inside_rect(1, 1, 9, 21, COLOR_BLACK), "");

	zassert_true(verify_color_inside_rect(0, 0, 10, 1, COLOR_BLUE));
	zassert_true(verify_color_inside_rect(0, 0, 1, 22, COLOR_BLUE));
	zassert_true(verify_color_inside_rect(10, 0, 1, 22, COLOR_BLUE));
	zassert_true(verify_color_inside_rect(0, 22, 10, 1, COLOR_BLUE));
}

ZTEST(draw_rect, test_draw_rect_at_0_0_color)
{
	struct cfb_position start = {0, 0};
	struct cfb_position end = {start.x + 10, start.y + 22};

	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0x4D, 0x75, 0xBA, 0));

	zassert_ok(cfb_draw_rect(fb, &start, &end), "");
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(0, 0, 11, 23, COLOR_BLACK), "");
	zassert_true(verify_color_inside_rect(1, 1, 9, 21, COLOR_BLACK), "");

	zassert_true(verify_color_inside_rect(0, 0, 10, 1, COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(0, 0, 1, 22, COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(10, 0, 1, 22, COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(0, 22, 10, 1, COLOR_TEST_COLOR));
}

ZTEST_SUITE(draw_rect, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

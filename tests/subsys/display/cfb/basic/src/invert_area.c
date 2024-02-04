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
static struct cfb_display disp;
static struct cfb_framebuffer *fb;

/**
 * Fill the buffer with 0 before running tests.
 */
static void cfb_test_before(void *text_fixture)
{
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_buf_size(dev),
	};

	memset(read_buffer, 0, sizeof(read_buffer));
	zassert_ok(display_write(dev, 0, 0, &desc, read_buffer));

	zassert_ok(display_blanking_off(dev));

	zassert_ok(cfb_display_init(&disp, dev, transfer_buf, CONFIG_TEST_CFB_TRANSFER_BUF_SIZE,
				    command_buf, CONFIG_TEST_CFB_COMMAND_BUF_SIZE));
	fb = cfb_display_get_framebuffer(&disp);
}

static void cfb_test_after(void *test_fixture)
{
	cfb_display_deinit(&disp);
}

ZTEST(invert_area, test_invert_area_whole_screen)
{
	zassert_ok(cfb_invert_area(fb, 0, 0, 320, 240));
	zassert_ok(cfb_finalize(fb), "cfb_finalize failed");

	zassert_true(verify_color_inside_rect(0, 0, 320, 240, COLOR_WHITE));
}

ZTEST(invert_area, test_invert_area_overlapped_2times)
{
	zassert_ok(cfb_invert_area(fb, 33, 37, 79, 77));
	zassert_ok(cfb_invert_area(fb, 100, 37, 53, 77));
	zassert_ok(cfb_finalize(fb), "cfb_finalize failed");

	zassert_true(verify_color_inside_rect(33, 37, 67, 77, COLOR_WHITE));
	zassert_true(verify_color_inside_rect(100, 37, 12, 77, COLOR_BLACK));
	zassert_true(verify_color_inside_rect(112, 37, 41, 77, COLOR_WHITE));
	zassert_true(verify_color_outside_rect(33, 37, 120, 77, COLOR_BLACK));
}

ZTEST(invert_area, test_invert_area_overlap_top_left)
{
	zassert_ok(cfb_invert_area(fb, -10, -10, 20, 20));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, 10, 10, COLOR_WHITE));
	zassert_true(verify_color_outside_rect(0, 0, 10, 10, COLOR_BLACK));
}

ZTEST(invert_area, test_invert_area_overlap_top_right)
{
	zassert_ok(cfb_invert_area(fb, display_width - 10, -10, 20, 20));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(display_width - 10, 0, 10, 10, COLOR_WHITE));
	zassert_true(verify_color_outside_rect(display_width - 10, 0, 10, 10, COLOR_BLACK));
}

ZTEST(invert_area, test_invert_area_overlap_bottom_left)
{
	zassert_ok(cfb_invert_area(fb, -10, display_height - 10, 20, 20));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, display_height - 10, 10, 10, COLOR_WHITE));
	zassert_true(verify_color_outside_rect(0, display_height - 10, 10, 10, COLOR_BLACK));
}

ZTEST(invert_area, test_invert_area_overlap_bottom_right)
{
	zassert_ok(cfb_invert_area(fb, display_width - 10, display_height - 10, 20, 20));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(display_width - 10, display_height - 10, 10, 10,
					      COLOR_WHITE));
	zassert_true(verify_color_outside_rect(display_width - 10, display_height - 10, 10, 10,
					       COLOR_BLACK));
}

ZTEST(invert_area, test_invert_area_outside_top_left)
{
	zassert_ok(cfb_invert_area(fb, -10, -10, 10, 10));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, COLOR_BLACK));
}

ZTEST(invert_area, test_invert_area_outside_bottom_right)
{
	zassert_ok(cfb_invert_area(fb, display_width, display_height, 20, 20));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, COLOR_BLACK));
}

ZTEST(invert_area, test_invert_area_color)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0x4D, 0x75, 0xBA, 0));
	zassert_ok(cfb_set_bg_color(fb, 0xAA, 0xAA, 0xAA, 0));

	zassert_ok(cfb_print(fb, " ", 40, 40));
	zassert_ok(cfb_invert_area(fb, 0, 0, display_width, 50));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, display_width, 40, COLOR_WHITE));
	zassert_true(verify_color_inside_rect(0, 40, 40, 10, COLOR_WHITE));
	zassert_true(verify_color_inside_rect(50, 40, 270, 10, COLOR_WHITE));
	zassert_true(verify_color_inside_rect(0, 56, display_width, 184, COLOR_BLACK));
	zassert_true(verify_color_inside_rect(0, 50, 40, 6, COLOR_BLACK));
	zassert_true(verify_color_inside_rect(50, 50, 270, 6, COLOR_BLACK));

	zassert_true(verify_color_inside_rect(41, 41, 8, 9, 0x00555555));
	zassert_true(verify_color_inside_rect(41, 50, 8, 5, COLOR_TEST_BG));

	zassert_true(verify_color_inside_rect(40, 40, 1, 10, ~COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(40, 40, 10, 1, ~COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(49, 40, 1, 10, ~COLOR_TEST_COLOR));

	zassert_true(verify_color_inside_rect(40, 50, 1, 6, COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(40, 55, 10, 1, COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(49, 50, 1, 6, COLOR_TEST_COLOR));
}

ZTEST_SUITE(invert_area, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

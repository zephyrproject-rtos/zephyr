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

LOG_MODULE_REGISTER(draw_text_rectspace1016, CONFIG_DISPLAY_LOG_LEVEL);

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
	uint8_t font_width;
	uint8_t font_height;
	bool font_found = false;

	memset(read_buffer, 0, sizeof(read_buffer));
	zassert_ok(display_write(dev, 0, 0, &desc, read_buffer));

	zassert_ok(display_blanking_off(dev));

	zassert_ok(cfb_display_init(&disp, dev, transfer_buf, CONFIG_TEST_CFB_TRANSFER_BUF_SIZE,
				    command_buf, CONFIG_TEST_CFB_COMMAND_BUF_SIZE));
	fb = cfb_display_get_framebuffer(&disp);

	for (int idx = 0; idx < cfb_get_numof_fonts(); idx++) {
		if (cfb_get_font_size(idx, &font_width, &font_height)) {
			break;
		}

		if (font_width == 10 && font_height == 16) {
			cfb_set_font(fb, idx);
			font_found = true;
			break;
		}
	}

	zassert_true(font_found);
}

static void cfb_test_after(void *test_fixture)
{
	cfb_display_deinit(&disp);
}

/*
 * normal rendering
 */
ZTEST(draw_text_rectspace1016, test_draw_text_at_0_0)
{
	zassert_ok(cfb_draw_text(fb, " ", 0, 0));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(0, 0, rectspace1016, 10, 16, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_1_1)
{
	zassert_ok(cfb_draw_text(fb, " ", 1, 1));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(1, 1, rectspace1016, 10, 16, COLOR_BLACK));
}

/*
 * around tile border
 */
ZTEST(draw_text_rectspace1016, test_draw_text_at_9_15)
{
	zassert_ok(cfb_draw_text(fb, " ", 9, 15));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(9, 15, rectspace1016, 10, 16, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_10_16)
{
	zassert_ok(cfb_draw_text(fb, " ", 10, 16));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(10, 16, rectspace1016, 10, 16, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_11_17)
{
	zassert_ok(cfb_draw_text(fb, " ", 11, 17));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(11, 17, rectspace1016, 10, 16, COLOR_BLACK));
}

/*
 * with kerning
 */
ZTEST(draw_text_rectspace1016, test_draw_text_at_0_0_kerning_3)
{
	cfb_set_kerning(fb, 3);
	zassert_ok(cfb_draw_text(fb, "  ", 0, 0));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(0, 0, kerning_3_2rectspace1016, 23, 16, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_1_1_kerning_3)
{
	cfb_set_kerning(fb, 3);
	zassert_ok(cfb_draw_text(fb, "  ", 1, 1));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(1, 1, kerning_3_2rectspace1016, 23, 16, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_9_15_kerning_3)
{
	cfb_set_kerning(fb, 3);
	zassert_ok(cfb_draw_text(fb, "  ", 9, 15));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(9, 15, kerning_3_2rectspace1016, 23, 16, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_10_16_kerning_3)
{
	cfb_set_kerning(fb, 3);
	zassert_ok(cfb_draw_text(fb, "  ", 10, 16));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(10, 16, kerning_3_2rectspace1016, 23, 16, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_11_17_kerning_3)
{
	cfb_set_kerning(fb, 3);
	zassert_ok(cfb_draw_text(fb, "  ", 11, 17));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(11, 17, kerning_3_2rectspace1016, 23, 16, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_kerning_3_within_right_border)
{
	cfb_set_kerning(fb, 3);
	zassert_ok(cfb_draw_text(fb, "  ", display_width - 23, 17));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image(display_width - 23, 17, kerning_3_2rectspace1016, 23, 16));
}

ZTEST(draw_text_rectspace1016, test_draw_text_kerning_3_over_right_border)
{
	cfb_set_kerning(fb, 3);
	zassert_ok(cfb_draw_text(fb, "  ", display_width - 22, 17));
	zassert_ok(cfb_finalize(fb));

	zassert_true(
		verify_image(display_width - 22, 17, kerning_3_rightclip_1_2rectspace1016, 22, 16));
}

ZTEST(draw_text_rectspace1016, test_draw_text_outside_top_left)
{
	zassert_ok(cfb_draw_text(fb, " ", -(10 - 3), -(16 - 4)));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image_and_bg(0, 0, outside_top_left, 3, 4, COLOR_BLACK));
}

ZTEST(draw_text_rectspace1016, test_draw_text_outside_top_right)
{
	zassert_ok(cfb_draw_text(fb, " ", display_width - 5, -(16 - 8)));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image(display_width - 5, 0, outside_top_right, 5, 8));
}

ZTEST(draw_text_rectspace1016, test_draw_text_outside_bottom_right)
{
	zassert_ok(cfb_draw_text(fb, " ", display_width - 3, display_height - 5));
	zassert_ok(cfb_finalize(fb));

	zassert_true(
		verify_image(display_width - 3, display_height - 5, outside_bottom_right, 3, 5));
}

ZTEST(draw_text_rectspace1016, test_draw_text_outside_bottom_left)
{
	zassert_ok(cfb_draw_text(fb, " ", -(10 - 3), display_height - 14));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_image(0, display_height - 14, outside_bottom_left, 3, 14));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_0_0_red)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0xFF, 0, 0, 0));
	zassert_ok(cfb_set_bg_color(fb, 0xAA, 0xAA, 0xAA, 0));

	zassert_ok(cfb_draw_text(fb, " ", 0, 0));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(0, 0, 10, 16, COLOR_BLACK), "");
	zassert_true(verify_color_inside_rect(1, 1, 8, 14, COLOR_BLACK), "");

	zassert_true(verify_color_inside_rect(0, 0, 10, 1, COLOR_RED));
	zassert_true(verify_color_inside_rect(0, 0, 1, 16, COLOR_RED));
	zassert_true(verify_color_inside_rect(9, 0, 1, 16, COLOR_RED));
	zassert_true(verify_color_inside_rect(0, 15, 10, 1, COLOR_RED));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_0_0_green)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0x0, 0xFF, 0, 0));
	zassert_ok(cfb_set_bg_color(fb, 0xAA, 0xAA, 0xAA, 0));

	zassert_ok(cfb_draw_text(fb, " ", 0, 0));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(0, 0, 10, 16, COLOR_BLACK), "");
	zassert_true(verify_color_inside_rect(1, 1, 8, 14, COLOR_BLACK), "");

	zassert_true(verify_color_inside_rect(0, 0, 10, 1, COLOR_GREEN));
	zassert_true(verify_color_inside_rect(0, 0, 1, 16, COLOR_GREEN));
	zassert_true(verify_color_inside_rect(9, 0, 1, 16, COLOR_GREEN));
	zassert_true(verify_color_inside_rect(0, 15, 10, 1, COLOR_GREEN));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_0_0_blue)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0, 0, 0xFF, 0));
	zassert_ok(cfb_set_bg_color(fb, 0xAA, 0xAA, 0xAA, 0));

	zassert_ok(cfb_draw_text(fb, " ", 0, 0));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(0, 0, 10, 16, COLOR_BLACK), "");
	zassert_true(verify_color_inside_rect(1, 1, 8, 14, COLOR_BLACK), "");

	zassert_true(verify_color_inside_rect(0, 0, 10, 1, COLOR_BLUE));
	zassert_true(verify_color_inside_rect(0, 0, 1, 16, COLOR_BLUE));
	zassert_true(verify_color_inside_rect(9, 0, 1, 16, COLOR_BLUE));
	zassert_true(verify_color_inside_rect(0, 15, 10, 1, COLOR_BLUE));
}

ZTEST(draw_text_rectspace1016, test_draw_text_at_0_0_color)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_fg_color(fb, 0x4D, 0x75, 0xBA, 0));
	zassert_ok(cfb_set_bg_color(fb, 0xAA, 0xAA, 0xAA, 0));

	zassert_ok(cfb_draw_text(fb, " ", 0, 0));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(0, 0, 10, 16, COLOR_BLACK), "");
	zassert_true(verify_color_inside_rect(1, 1, 8, 14, COLOR_BLACK), "");

	zassert_true(verify_color_inside_rect(0, 0, 10, 1, COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(0, 0, 1, 16, COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(9, 0, 1, 16, COLOR_TEST_COLOR));
	zassert_true(verify_color_inside_rect(0, 15, 10, 1, COLOR_TEST_COLOR));
}

ZTEST_SUITE(draw_text_rectspace1016, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

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

LOG_MODULE_REGISTER(draw_text_dot0101, CONFIG_DISPLAY_LOG_LEVEL);

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
	uint8_t font_width;
	uint8_t font_height;
	bool font_found = false;

	memset(read_buffer, 0, sizeof(read_buffer));
	zassert_ok(display_write(dev, 0, 0, &desc, read_buffer));

	zassert_ok(cfb_framebuffer_init(dev));

	for (int idx = 0; idx < cfb_get_numof_fonts(dev); idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}

		if (font_width == 1 && font_height == 1) {
			cfb_framebuffer_set_font(dev, idx);
			font_found = true;
			break;
		}
	}

	zassert_true(font_found);
}

static void cfb_test_after(void *test_fixture)
{
	cfb_framebuffer_deinit(dev);
}

/*
 * normal rendering
 */
ZTEST(draw_text_dot0101, test_draw_text_at_0_0)
{
	zassert_ok(cfb_draw_text(dev, "1", 0, 0));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(0, 0, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_at_1_1)
{
	zassert_ok(cfb_draw_text(dev, "1", 1, 1));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(1, 1, 1, 0));
}

/*
 * around tile border
 */
ZTEST(draw_text_dot0101, test_draw_text_at_9_15)
{
	zassert_ok(cfb_draw_text(dev, "1", 9, 15));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(9, 15, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_at_10_16)
{
	zassert_ok(cfb_draw_text(dev, "1", 10, 16));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(10, 16, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_at_11_17)
{
	zassert_ok(cfb_draw_text(dev, "1", 11, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel_and_bg(11, 17, 1, 0));
}

/*
 * kerning
 */
ZTEST(draw_text_dot0101, test_draw_text_at_0_0_kerning_3)
{
	cfb_set_kerning(dev, 3);
	zassert_ok(cfb_draw_text(dev, "11", 0, 0));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(0, 0, kerning_3_2dot0101, 5, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_at_1_1_kerning_3)
{
	cfb_set_kerning(dev, 3);
	zassert_ok(cfb_draw_text(dev, "11", 1, 1));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(1, 1, kerning_3_2dot0101, 5, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_at_9_15_kerning_3)
{
	cfb_set_kerning(dev, 3);
	zassert_ok(cfb_draw_text(dev, "11", 9, 15));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(9, 15, kerning_3_2dot0101, 5, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_at_10_16_kerning_3)
{
	cfb_set_kerning(dev, 3);
	zassert_ok(cfb_draw_text(dev, "11", 10, 16));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(10, 16, kerning_3_2dot0101, 5, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_at_11_17_kerning_3)
{
	cfb_set_kerning(dev, 3);
	zassert_ok(cfb_draw_text(dev, "11", 11, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(11, 17, kerning_3_2dot0101, 5, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_at_right_border_17_kerning_3)
{
	cfb_set_kerning(dev, 3);
	zassert_ok(cfb_draw_text(dev, "11", display_width - 5, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(display_width - 5, 17, kerning_3_2dot0101, 5, 1, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_kerning_3_over_border)
{
	cfb_set_kerning(dev, 3);
	zassert_ok(cfb_draw_text(dev, "11", display_width - 4, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_pixel(display_width - 4, 17, 0xFFFFFF));
	zassert_true(verify_pixel(display_width - 3, 17, 0x0));
	zassert_true(verify_pixel(display_width - 2, 17, 0x0));
	zassert_true(verify_pixel(display_width - 1, 17, 0x0));
	zassert_true(verify_pixel(0, 18, 0x0));
}

ZTEST(draw_text_dot0101, test_draw_text_outside_top_left)
{
	zassert_ok(cfb_draw_text(dev, "1", 0, -1));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_outside_top_right)
{
	zassert_ok(cfb_draw_text(dev, "1", display_width, 0));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_outside_bottom_right)
{
	zassert_ok(cfb_draw_text(dev, "1", 0, display_height));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(draw_text_dot0101, test_draw_text_outside_bottom_left)
{
	zassert_ok(cfb_draw_text(dev, "1", display_width, -1));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST_SUITE(draw_text_dot0101, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

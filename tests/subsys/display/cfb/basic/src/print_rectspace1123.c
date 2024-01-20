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

LOG_MODULE_REGISTER(print_rectspace1123, CONFIG_DISPLAY_LOG_LEVEL);

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

	uint8_t font_width;
	uint8_t font_height;
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width / 8,
	};

	for (int idx = 0; idx < cfb_get_numof_fonts(dev); idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}

		if (font_width == 11 && font_height == 23) {
			cfb_framebuffer_set_font(dev, idx);
			break;
		}
	}

	memset(read_buffer, 0, sizeof(read_buffer));
	display_write(dev, 0, 0, &desc, read_buffer);

	display_blanking_off(dev);

	cfb_framebuffer_clear(dev, false);
}

/*
 * normal rendering
 */
ZTEST(print_rectspace1123, test_print_at_0_0)
{
	zassert_ok(cfb_print(dev, " ", 0, 0));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(0, 0, rectspace1123, 11, 23, 0));
}

ZTEST(print_rectspace1123, test_print_at_1_1)
{
	zassert_ok(cfb_print(dev, " ", 1, 1));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(1, 1, rectspace1123, 11, 23, 0));
}

/*
 * around tile border
 */
ZTEST(print_rectspace1123, test_print_at_9_15)
{
	zassert_ok(cfb_print(dev, " ", 9, 15));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(9, 15, rectspace1123, 11, 23, 0));
}

ZTEST(print_rectspace1123, test_print_at_10_16)
{
	zassert_ok(cfb_print(dev, " ", 10, 16));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(10, 16, rectspace1123, 11, 23, 0));
}

ZTEST(print_rectspace1123, test_print_at_11_17)
{
	zassert_ok(cfb_print(dev, " ", 11, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(11, 17, rectspace1123, 11, 23, 0));
}

/*
 * kerning
 */
ZTEST(print_rectspace1123, test_print_at_0_0_kerning_1)
{
	cfb_set_kerning(dev, 1);
	zassert_ok(cfb_print(dev, "  ", 0, 0));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(0, 0, kerning_1_2rectspace1123, 23, 23, 0));
}

ZTEST(print_rectspace1123, test_print_at_1_1_kerning_1)
{
	cfb_set_kerning(dev, 1);
	zassert_ok(cfb_print(dev, "  ", 1, 1));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(1, 1, kerning_1_2rectspace1123, 23, 23, 0));
}

ZTEST(print_rectspace1123, test_print_at_9_15_kerning_1)
{
	cfb_set_kerning(dev, 1);
	zassert_ok(cfb_print(dev, "  ", 9, 15));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(9, 15, kerning_1_2rectspace1123, 23, 23, 0));
}

ZTEST(print_rectspace1123, test_print_at_10_16_kerning_1)
{
	cfb_set_kerning(dev, 1);
	zassert_ok(cfb_print(dev, "  ", 10, 16));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(10, 16, kerning_1_2rectspace1123, 23, 23, 0));
}

ZTEST(print_rectspace1123, test_print_at_11_17_kerning_1)
{
	cfb_set_kerning(dev, 1);
	zassert_ok(cfb_print(dev, "  ", 11, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(11, 17, kerning_1_2rectspace1123, 23, 23, 0));
}

ZTEST(print_rectspace1123, test_print_at_right_border_17_kerning_1)
{
	cfb_set_kerning(dev, 1);
	zassert_ok(cfb_print(dev, "  ", display_width - 23, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image(display_width - 23, 17, kerning_1_2rectspace1123, 23, 23));
}

ZTEST(print_rectspace1123, test_print_at_right_border_plus1_kerning_1)
{
	cfb_set_kerning(dev, 1);
	zassert_ok(cfb_print(dev, "  ", display_width - 22, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image(display_width - 22, 17, rectspace1123, 11, 23));
	zassert_true(verify_image(0, 40, rectspace1123, 11, 23));
}

ZTEST(print_rectspace1123, test_print_outside_top_left)
{
	zassert_ok(cfb_print(dev, " ", -(11 - 3), -(23 - 4)));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(0, 0, outside_top_left, 3, 4, 0));
}

ZTEST(print_rectspace1123, test_print_outside_top_right)
{
	zassert_ok(cfb_print(dev, " ", display_width - 5, -(23 - 8)));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image_and_bg(0, 8, rectspace1123, 11, 23, 0));
}

ZTEST(print_rectspace1123, test_print_outside_bottom_right)
{
	zassert_ok(cfb_print(dev, " ", display_width - 3, display_height - 5));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, 0));
}

ZTEST(print_rectspace1123, test_print_outside_bottom_left)
{
	zassert_ok(cfb_print(dev, " ", -(11 - 3), display_height - 14));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image(0, display_height - 14, outside_bottom_left, 3, 14));
}

ZTEST(print_rectspace1123, test_print_wrap_to_3_lines)
{
	zassert_ok(
		cfb_print(dev, "                                                     ", 160, 17));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_image(160, 17, kerning_1_12rectspace1123, 155, 23));
	zassert_true(verify_image(0, 40, kerning_1_12rectspace1123, 155, 23));
	zassert_true(verify_image(156, 40, kerning_1_12rectspace1123, 155, 23));
	zassert_true(verify_image(0, 63, kerning_1_12rectspace1123, 155, 23));
	zassert_true(verify_image(12, 63, kerning_1_12rectspace1123, 155, 23));
}

ZTEST_SUITE(print_rectspace1123, NULL, cfb_test_init, cfb_test_before, NULL, NULL);

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

LOG_MODULE_REGISTER(clear, CONFIG_DISPLAY_LOG_LEVEL);


static struct cfb_display *disp;
static struct cfb_framebuffer *fb;

/**
 * Fill the buffer with 0xAA before running tests.
 */
static void cfb_test_before(void *text_fixture)
{
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_buf_size(dev),
	};

	disp = display_init();
	fb = cfb_display_get_framebuffer(disp);

	memset(read_buffer, 0xAA, sizeof(read_buffer));
	zassert_ok(display_write(dev, 0, 0, &desc, read_buffer));
}

static void cfb_test_after(void *test_fixture)
{
	display_deinit(disp);
}

ZTEST(clear, test_clear_false)
{
	zassert_ok(cfb_clear(fb, false));

	/* checking memory not updated */
	zassert_false(verify_color_inside_rect(0, 0, display_width, display_height, COLOR_BLACK));
}

ZTEST(clear, test_clear_true)
{
	zassert_ok(cfb_clear(fb, true));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, COLOR_BLACK));
}

ZTEST(clear, test_clear_red_true)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_bg_color(fb, 0xFF, 0x00, 0x00, 0xFF));
	zassert_ok(cfb_clear(fb, true));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, COLOR_RED));
}

ZTEST(clear, test_clear_green_true)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_bg_color(fb, 0x0, 0xFF, 0x00, 0xFF));
	zassert_ok(cfb_clear(fb, true));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, COLOR_GREEN));
}

ZTEST(clear, test_clear_blue_true)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_bg_color(fb, 0x0, 0x00, 0xFF, 0xFF));
	zassert_ok(cfb_clear(fb, true));

	zassert_true(verify_color_inside_rect(0, 0, display_width, display_height, COLOR_BLUE));
}

ZTEST(clear, test_clear_color_true)
{
	SKIP_MONO_DISP();

	zassert_ok(cfb_set_bg_color(fb, 0x4D, 0x75, 0xBA, 0));
	zassert_ok(cfb_clear(fb, true));

	zassert_true(
		verify_color_inside_rect(0, 0, display_width, display_height, COLOR_TEST_COLOR));
}

ZTEST_SUITE(clear, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

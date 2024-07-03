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

LOG_MODULE_REGISTER(invert, CONFIG_DISPLAY_LOG_LEVEL);

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

ZTEST(invert, test_invert)
{
	zassert_ok(cfb_framebuffer_invert(dev));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_inside_rect(0, 0, 320, 240, 0xFFFFFF));
}

ZTEST(invert, test_invert_contents)
{
	zassert_ok(cfb_invert_area(dev, 10, 10, 10, 10));
	zassert_ok(cfb_framebuffer_finalize(dev));
	zassert_true(verify_color_outside_rect(10, 10, 10, 10, 0));
	zassert_true(verify_color_inside_rect(10, 10, 10, 10, 0xFFFFFF));

	zassert_ok(cfb_framebuffer_invert(dev));
	zassert_ok(cfb_framebuffer_finalize(dev));

	zassert_true(verify_color_outside_rect(10, 10, 10, 10, 0xFFFFFF));
}

ZTEST_SUITE(invert, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

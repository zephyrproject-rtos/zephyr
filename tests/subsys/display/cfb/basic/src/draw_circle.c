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

LOG_MODULE_REGISTER(draw_circle, CONFIG_DISPLAY_LOG_LEVEL);

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

/*
 * normal rendering
 */
ZTEST(draw_circle, test_draw_circle_10_at_0_0)
{
	struct cfb_position center = {0, 0};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(0, 0, circle10, 20, 20), "");
}

ZTEST(draw_circle, test_draw_circle_10_at_1_1)
{
	struct cfb_position center = {1, 1};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(1, 1, circle10, 20, 20), "");
}

/* tile border case */
ZTEST(draw_circle, test_draw_circle_10_at_9_15)
{
	struct cfb_position center = {9, 15};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(9, 15, circle10, 20, 20), "");
}

ZTEST(draw_circle, test_draw_circle_10_at_10_16)
{
	struct cfb_position center = {10, 16};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(10, 16, circle10, 20, 20), "");
}

ZTEST(draw_circle, test_draw_circle_10_at_11_17)
{
	struct cfb_position center = {11, 17};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(11, 17, circle10, 20, 20), "");
}

/*
 * Case of including coordinates outside the area
 */
ZTEST(draw_circle, test_draw_circle_10_outside_top_left)
{
	struct cfb_position center = {-(20 - 3), -(20 - 4)};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(0, 0, outside_top_left, 3, 4), "");
}

ZTEST(draw_circle, test_draw_circle_10_outside_top_right)
{
	struct cfb_position center = {display_width - 5, -(20 - 8)};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(display_width - 5, 0, outside_top_right, 5, 8), "");
}

ZTEST(draw_circle, test_draw_circle_10_outside_bottom_right)
{
	struct cfb_position center = {display_width - 3, display_height - 5};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(
		verify_image(display_width - 3, display_height - 5, outside_bottom_right, 3, 5),
		"");
}

ZTEST(draw_circle, test_draw_circle_10_outside_bottom_left)
{
	struct cfb_position center = {-(20 - 3), display_height - 14};
	uint16_t radius = 10;

	zassert_ok(cfb_draw_circle(dev, &center, radius), "");
	zassert_ok(cfb_framebuffer_finalize(dev), "");

	zassert_true(verify_image(0, display_height - 14, outside_bottom_left, 3, 14), "");
}

ZTEST_SUITE(draw_circle, NULL, NULL, cfb_test_before, cfb_test_after, NULL);

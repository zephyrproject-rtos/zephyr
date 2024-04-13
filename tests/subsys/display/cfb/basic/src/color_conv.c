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

LOG_MODULE_REGISTER(color_conv, CONFIG_DISPLAY_LOG_LEVEL);

void test_color_to_rgba(const enum display_pixel_format pixel_format, uint32_t color, uint8_t *r,
			uint8_t *g, uint8_t *b, uint8_t *a);
uint32_t test_rgba_to_color(const enum display_pixel_format pixel_format, uint8_t r, uint8_t g,
			    uint8_t b, uint8_t a);

ZTEST(color_conv, test_color_to_rgba_0_mono01)
{
	const enum display_pixel_format format = PIXEL_FORMAT_MONO01;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0, r);
	zassert_equal(0, g);
	zassert_equal(0, b);
	zassert_equal(0, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_1_mono01)
{
	const enum display_pixel_format format = PIXEL_FORMAT_MONO01;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 1;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0xFF, r);
	zassert_equal(0xFF, g);
	zassert_equal(0xFF, b);
	zassert_equal(0xFF, a);

	zassert_equal(0xFFFFFFFF, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0xFFFFFFFF_mono01)
{
	const enum display_pixel_format format = PIXEL_FORMAT_MONO01;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0xFFFFFFFF;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0xFF, r);
	zassert_equal(0xFF, g);
	zassert_equal(0xFF, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0_mono10)
{
	const enum display_pixel_format format = PIXEL_FORMAT_MONO10;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0xFF, r);
	zassert_equal(0xFF, g);
	zassert_equal(0xFF, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_1_mono10)
{
	const enum display_pixel_format format = PIXEL_FORMAT_MONO10;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 1;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0, r);
	zassert_equal(0, g);
	zassert_equal(0, b);
	zassert_equal(0, a);

	zassert_equal(0xFFFFFFFF, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0xFFFFFFFF_mono10)
{
	const enum display_pixel_format format = PIXEL_FORMAT_MONO10;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0xFFFFFFFF;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0, r);
	zassert_equal(0, g);
	zassert_equal(0, b);
	zassert_equal(0, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0_rgb888)
{
	const enum display_pixel_format format = PIXEL_FORMAT_RGB_888;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0, r);
	zassert_equal(0, g);
	zassert_equal(0, b);
	zassert_equal(0xFF, a);

	zassert_equal(0xFF000000, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_1_rgb888)
{
	const enum display_pixel_format format = PIXEL_FORMAT_RGB_888;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0xFF010203;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(1, r);
	zassert_equal(2, g);
	zassert_equal(3, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0xFFFFFFFF_rgb888)
{
	const enum display_pixel_format format = PIXEL_FORMAT_RGB_888;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0xFFFFFFFF;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0xFF, r);
	zassert_equal(0xFF, g);
	zassert_equal(0xFF, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0_argb888)
{
	const enum display_pixel_format format = PIXEL_FORMAT_ARGB_8888;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0, r);
	zassert_equal(0, g);
	zassert_equal(0, b);
	zassert_equal(0, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_1_argb888)
{
	const enum display_pixel_format format = PIXEL_FORMAT_ARGB_8888;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0x05010203;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(1, r);
	zassert_equal(2, g);
	zassert_equal(3, b);
	zassert_equal(5, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0xFFFFFFFF_argb8888)
{
	const enum display_pixel_format format = PIXEL_FORMAT_ARGB_8888;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0xFFFFFFFF;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0xFF, r);
	zassert_equal(0xFF, g);
	zassert_equal(0xFF, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0_rgb565)
{
	const enum display_pixel_format format = PIXEL_FORMAT_RGB_565;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0, r);
	zassert_equal(0, g);
	zassert_equal(0, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_1_rgb565)
{
	const enum display_pixel_format format = PIXEL_FORMAT_RGB_565;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0x2108;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(8, r);
	zassert_equal(4, g);
	zassert_equal(8, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0xFFFFFFFF_rgb565)
{
	const enum display_pixel_format format = PIXEL_FORMAT_RGB_565;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0xFFFFFFFF;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0xF8, r);
	zassert_equal(0xFC, g);
	zassert_equal(0xF8, b);
	zassert_equal(0xFF, a);

	zassert_equal(0xFFFF, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0_bgr565)
{
	const enum display_pixel_format format = PIXEL_FORMAT_BGR_565;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0, r);
	zassert_equal(0, g);
	zassert_equal(0, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_1_bgr565)
{
	const enum display_pixel_format format = PIXEL_FORMAT_BGR_565;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0x0821;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(8, r);
	zassert_equal(4, g);
	zassert_equal(8, b);
	zassert_equal(0xFF, a);

	zassert_equal(color, test_rgba_to_color(format, r, g, b, a));
}

ZTEST(color_conv, test_color_to_rgba_0xFFFFFFFF_bgr565)
{
	const enum display_pixel_format format = PIXEL_FORMAT_BGR_565;
	uint8_t r = 0, g = 0, b = 0, a = 0;
	uint32_t color = 0xFFFFFFFF;

	test_color_to_rgba(format, color, &r, &g, &b, &a);

	zassert_equal(0xF8, r);
	zassert_equal(0xFC, g);
	zassert_equal(0xF8, b);
	zassert_equal(0xFF, a);

	zassert_equal(0xFFFF, test_rgba_to_color(format, r, g, b, a));
}

ZTEST_SUITE(color_conv, NULL, NULL, NULL, NULL, NULL);

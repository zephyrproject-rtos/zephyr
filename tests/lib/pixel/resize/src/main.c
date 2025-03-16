/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/pixel/image.h>
#include <zephyr/pixel/print.h>

#define WIDTH_IN 6
#define HEIGHT_IN 10
#define PITCH_IN (WIDTH_IN * 3)

#define WIDTH_OUT 4
#define HEIGHT_OUT 22
#define PITCH_OUT (WIDTH_OUT * 3)

#define ERROR_MARGIN 9

/* Input/output buffers */
static uint8_t rgb24frame_in[WIDTH_IN * HEIGHT_IN * 3];
static uint8_t rgb24frame_out[WIDTH_OUT * HEIGHT_OUT * 3];

static void test_resize(uint32_t fourcc)
{
	struct pixel_image img;
	size_t w = WIDTH_OUT;
	size_t h = HEIGHT_OUT;
	size_t p = PITCH_OUT;
	int ret;

	pixel_image_from_buffer(&img, rgb24frame_in, sizeof(rgb24frame_in), WIDTH_IN, HEIGHT_IN,
				VIDEO_PIX_FMT_RGB24);

	printf("input:\n");
	pixel_image_print_truecolor(&img);

	ret = pixel_image_convert(&img, fourcc);
	zassert_ok(ret);
	ret = pixel_image_resize(&img, WIDTH_OUT, HEIGHT_OUT);
	zassert_ok(ret);
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_RGB24);
	zassert_ok(ret);
	ret = pixel_image_to_buffer(&img, rgb24frame_out, sizeof(rgb24frame_out));
	zassert_ok(ret);

	printf("output:\n");
	pixel_image_print_truecolor(&img);

	/* Test top left quadramt */
	zassert_within(rgb24frame_out[(0) * p + (0) * 3 + 0], 0x00, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(0) * p + (0) * 3 + 1], 0x00, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(0) * p + (0) * 3 + 2], 0x7f, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 - 1) * p + (w / 2 - 1) * 3 + 0], 0x00, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 - 1) * p + (w / 2 - 1) * 3 + 1], 0x00, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 - 1) * p + (w / 2 - 1) * 3 + 2], 0x7f, ERROR_MARGIN);

	/* Test bottom left quadrant */
	zassert_within(rgb24frame_out[(h - 1) * p + (0) * 3 + 0], 0x00, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h - 1) * p + (0) * 3 + 1], 0xff, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h - 1) * p + (0) * 3 + 2], 0x7f, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 + 1) * p + (w / 2 - 1) * 3 + 0], 0x00, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 + 1) * p + (w / 2 - 1) * 3 + 1], 0xff, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 + 1) * p + (w / 2 - 1) * 3 + 2], 0x7f, ERROR_MARGIN);

	/* Test top right quadrant */
	zassert_within(rgb24frame_out[(0) * p + (w - 1) * 3 + 0], 0xff, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(0) * p + (w - 1) * 3 + 1], 0x00, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(0) * p + (w - 1) * 3 + 2], 0x7f, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 - 1) * p + (w / 2 + 1) * 3 + 0], 0xff, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 - 1) * p + (w / 2 + 1) * 3 + 1], 0x00, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 - 1) * p + (w / 2 + 1) * 3 + 2], 0x7f, ERROR_MARGIN);

	/* Test bottom right quadrant */
	zassert_within(rgb24frame_out[(h - 1) * p + (w - 1) * 3 + 0], 0xff, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h - 1) * p + (w - 1) * 3 + 1], 0xff, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h - 1) * p + (w - 1) * 3 + 2], 0x7f, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 + 1) * p + (w / 2 + 1) * 3 + 0], 0xff, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 + 1) * p + (w / 2 + 1) * 3 + 1], 0xff, ERROR_MARGIN);
	zassert_within(rgb24frame_out[(h / 2 + 1) * p + (w / 2 + 1) * 3 + 2], 0x7f, ERROR_MARGIN);
}

ZTEST(lib_pixel_resize, test_pixel_resize_operation)
{
	/* Generate test input data */
	for (uint16_t h = 0; h < HEIGHT_IN; h++) {
		for (uint16_t w = 0; w < WIDTH_IN; w++) {
			rgb24frame_in[h * PITCH_IN + w * 3 + 0] = w < WIDTH_IN / 2 ? 0x00 : 0xff;
			rgb24frame_in[h * PITCH_IN + w * 3 + 1] = h < HEIGHT_IN / 2 ? 0x00 : 0xff;
			rgb24frame_in[h * PITCH_IN + w * 3 + 2] = 0x7f;
		}
	}

	test_resize(VIDEO_PIX_FMT_RGB24);
	test_resize(VIDEO_PIX_FMT_RGB565);
	test_resize(VIDEO_PIX_FMT_RGB565X);
}

ZTEST_SUITE(lib_pixel_resize, NULL, NULL, NULL, NULL, NULL);

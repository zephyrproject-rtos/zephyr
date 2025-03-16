/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/pixel/resize.h>
#include <zephyr/pixel/formats.h>
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

/* Stream conversion steps */
static PIXEL_RGB24STREAM_TO_RGB565BESTREAM(step_rgb24_to_rgb565be, WIDTH_IN, HEIGHT_IN);
static PIXEL_RGB24STREAM_TO_RGB565LESTREAM(step_rgb24_to_rgb565le, WIDTH_IN, HEIGHT_IN);
static PIXEL_RGB565BESTREAM_TO_RGB24STREAM(step_rgb565be_to_rgb24, WIDTH_OUT, HEIGHT_OUT);
static PIXEL_RGB565LESTREAM_TO_RGB24STREAM(step_rgb565le_to_rgb24, WIDTH_OUT, HEIGHT_OUT);
static PIXEL_SUBSAMPLE_RGB24STREAM(step_subsample_rgb24, WIDTH_IN, HEIGHT_IN);
static PIXEL_SUBSAMPLE_RGB565STREAM(step_subsample_rgb565, WIDTH_IN, HEIGHT_IN);

struct pixel_stream root = {0};

static void test_resize(void)
{
	printf("input:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_in, sizeof(rgb24frame_in), WIDTH_IN, HEIGHT_IN);

	printf("output:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_out, sizeof(rgb24frame_out), WIDTH_OUT,
					 HEIGHT_OUT);

	size_t w = WIDTH_OUT;
	size_t h = HEIGHT_OUT;
	size_t p = PITCH_OUT;

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

ZTEST(lib_pixel_resize, test_pixel_resize_stream)
{
	/* Generate test input data */
	for (uint16_t h = 0; h < HEIGHT_IN; h++) {
		for (uint16_t w = 0; w < WIDTH_IN; w++) {
			rgb24frame_in[h * PITCH_IN + w * 3 + 0] = w < WIDTH_IN / 2 ? 0x00 : 0xff;
			rgb24frame_in[h * PITCH_IN + w * 3 + 1] = h < HEIGHT_IN / 2 ? 0x00 : 0xff;
			rgb24frame_in[h * PITCH_IN + w * 3 + 2] = 0x7f;
		}
	}

	/* RGB24 */
	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH_IN,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH_OUT,
				   &step_subsample_rgb24, NULL);
	test_resize();

	/* RGB565LE */
	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH_IN,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH_OUT,
				   &step_rgb24_to_rgb565le, &step_subsample_rgb565,
				   &step_rgb565le_to_rgb24, NULL);
	test_resize();

	/* RGB565BE */
	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH_IN,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH_OUT,
				   &step_rgb24_to_rgb565be, &step_subsample_rgb565,
				   &step_rgb565be_to_rgb24, NULL);
	test_resize();
}

ZTEST_SUITE(lib_pixel_resize, NULL, NULL, NULL, NULL, NULL);

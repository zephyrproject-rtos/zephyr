/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/pixel/kernel.h>
#include <zephyr/pixel/print.h>

#define WIDTH 20
#define HEIGHT 20

/* Input/output buffers */
static uint8_t rgb24frame_in[WIDTH * HEIGHT * 3];
static uint8_t rgb24frame_out[WIDTH * HEIGHT * 3];

/* Stream conversion steps */
static PIXEL_IDENTITY_RGB24STREAM_3X3(step_identity_rgb24_3x3, WIDTH, HEIGHT);
static PIXEL_IDENTITY_RGB24STREAM_5X5(step_identity_rgb24_5x5, WIDTH, HEIGHT);
static PIXEL_MEDIAN_RGB24STREAM_3X3(step_median_rgb24_3x3, WIDTH, HEIGHT);
static PIXEL_MEDIAN_RGB24STREAM_5X5(step_median_rgb24_5x5, WIDTH, HEIGHT);
static PIXEL_GAUSSIANBLUR_RGB24STREAM_5X5(step_gaussianblur_rgb24_5x5, WIDTH, HEIGHT);
static PIXEL_GAUSSIANBLUR_RGB24STREAM_3X3(step_gaussianblur_rgb24_3x3, WIDTH, HEIGHT);

static void test_identity(void)
{
	printf("input:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_in, sizeof(rgb24frame_in), WIDTH, HEIGHT);

	printf("output:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_out, sizeof(rgb24frame_out), WIDTH, HEIGHT);

	for (uint16_t h = 0; h < HEIGHT; h++) {
		for (uint16_t w = 0; w < WIDTH; w++) {
			size_t i = h * WIDTH * 3 + w * 3;

			zassert_equal(rgb24frame_out[i + 0], rgb24frame_in[i + 0],
				      "channel R, row %u, col %u", h, w);
			zassert_equal(rgb24frame_out[i + 1], rgb24frame_in[i + 1],
				      "channel G, row %u, col %u", h, w);
			zassert_equal(rgb24frame_out[i + 2], rgb24frame_in[i + 2],
				      "channel B, row %u, col %u", h, w);
		}
	}
}

static void test_median(void)
{
	printf("input:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_in, sizeof(rgb24frame_in), WIDTH, HEIGHT);

	printf("output:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_out, sizeof(rgb24frame_out), WIDTH, HEIGHT);


	for (uint16_t h = 0; h < HEIGHT; h++) {
		uint16_t w = 0;

		/* Left half */
		for (; w < WIDTH / 2 - 1; w++) {
			size_t i = h * WIDTH * 3 + w * 3;

			zassert_equal(rgb24frame_out[i + 0], rgb24frame_out[i + 3],
				      "channel R, row %u, col %u", h, w);
			zassert_equal(rgb24frame_out[i + 1], rgb24frame_out[i + 4],
				      "channel G, row %u, col %u", h, w);
			zassert_equal(rgb24frame_out[i + 2], rgb24frame_out[i + 5],
				      "channel B, row %u, col %u", h, w);
		}

		/* Left right */
		for (; w < WIDTH / 2 - 1; w++) {
			size_t i = h * WIDTH * 3 + w * 3;

			zassert_equal(rgb24frame_out[i + 0], rgb24frame_out[i + 3],
				      "channel R, row %u, col %u", h, w);
			zassert_equal(rgb24frame_out[i + 1], rgb24frame_out[i + 4],
				      "channel G, row %u, col %u", h, w);
			zassert_equal(rgb24frame_out[i + 2], rgb24frame_out[i + 5],
				      "channel B, row %u, col %u", h, w);
		}
	}
}

static void test_blur(int blur_margin)
{
	printf("input:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_in, sizeof(rgb24frame_in), WIDTH, HEIGHT);

	printf("output:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_out, sizeof(rgb24frame_out), WIDTH, HEIGHT);


	for (uint16_t h = 0; h < HEIGHT; h++) {
		uint16_t w = 0;

		for (; w < WIDTH - 1; w++) {
			size_t i = h * WIDTH * 3 + w * 3;

			zassert_within(rgb24frame_out[i + 0], rgb24frame_out[i + 3], blur_margin,
				      "channel R, row %u, col %u", h, w);
			zassert_within(rgb24frame_out[i + 1], rgb24frame_out[i + 4], blur_margin,
				      "channel G, row %u, col %u", h, w);
			zassert_within(rgb24frame_out[i + 2], rgb24frame_out[i + 5], blur_margin,
				      "channel B, row %u, col %u", h, w);
		}
	}
}

ZTEST(lib_pixel_kernel, test_pixel_identity_kernel)
{
	/* Generate test input data */
	for (uint16_t h = 0; h < HEIGHT; h++) {
		for (uint16_t w = 0; w < WIDTH; w++) {
			rgb24frame_in[h * WIDTH * 3 + w * 3 + 0] = w < WIDTH / 2 ? 0x00 : 0xff;
			rgb24frame_in[h * WIDTH * 3 + w * 3 + 1] = (h % 3 + w % 3) / 4 * 0xff;
			rgb24frame_in[h * WIDTH * 3 + w * 3 + 2] = h * 0xff / HEIGHT;
		}
	}

	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH,
				   &step_identity_rgb24_3x3, &step_identity_rgb24_5x5, NULL);
	test_identity();

	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH,
				   &step_median_rgb24_3x3, NULL);
	test_median();

	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH,
				   &step_median_rgb24_5x5, NULL);

	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH,
				   &step_gaussianblur_rgb24_3x3, NULL);
	test_blur(128);

	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH,
				   &step_gaussianblur_rgb24_5x5, NULL);
	test_blur(96);
}

ZTEST_SUITE(lib_pixel_kernel, NULL, NULL, NULL, NULL, NULL);

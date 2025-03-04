/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/pixel/image.h>
#include <zephyr/pixel/kernel.h>
#include <zephyr/pixel/print.h>

#define WIDTH 20
#define HEIGHT 20

/* Input/output buffers */
static uint8_t rgb24frame_in[WIDTH * HEIGHT * 3];
static uint8_t rgb24frame_out[WIDTH * HEIGHT * 3];

static void run_kernel(uint32_t kernel_type, uint32_t kernel_size)
{
	struct pixel_image img;
	int ret;

	pixel_image_from_buffer(&img, rgb24frame_in, sizeof(rgb24frame_in), WIDTH, HEIGHT,
				VIDEO_PIX_FMT_RGB24);

	printf("input:\n");
	pixel_image_print_truecolor(&img);

	ret = pixel_image_kernel(&img, kernel_type, kernel_size);
	zassert_ok(ret);

	ret = pixel_image_to_buffer(&img, rgb24frame_out, sizeof(rgb24frame_out));
	zassert_ok(ret);

	printf("output:\n");
	pixel_image_print_truecolor(&img);
}

static void test_identity(uint32_t kernel_size)
{
	run_kernel(PIXEL_KERNEL_IDENTITY, kernel_size);

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

static void test_median(uint32_t kernel_size)
{
	run_kernel(PIXEL_KERNEL_DENOISE, kernel_size);

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

static void test_blur(uint32_t kernel_size, int blur_margin)
{
	run_kernel(PIXEL_KERNEL_GAUSSIAN_BLUR, kernel_size);

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

	test_identity(3);
	test_identity(5);

	test_median(3);
	test_median(5);

	test_blur(3, 128);
	test_blur(5, 96);
}

ZTEST_SUITE(lib_pixel_kernel, NULL, NULL, NULL, NULL, NULL);

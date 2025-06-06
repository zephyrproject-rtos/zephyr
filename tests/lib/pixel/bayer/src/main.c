/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/video.h>
#include <zephyr/pixel/print.h>
#include <zephyr/pixel/image.h>

#define WIDTH 16
#define HEIGHT 16

#define ERROR_MARGIN 13

static uint8_t bayerframe_in[WIDTH * HEIGHT * 1];
static uint8_t rgb24frame_out[WIDTH * HEIGHT * 3];

void test_bayer(uint32_t fourcc, uint32_t window_size, uint32_t expected_color)
{
	uint8_t r = expected_color >> 16;
	uint8_t g = expected_color >> 8;
	uint8_t b = expected_color >> 0;
	struct pixel_image img;
	int ret;

	pixel_image_from_buffer(&img, bayerframe_in, sizeof(bayerframe_in), WIDTH, HEIGHT, fourcc);

	printf("input:\n");
	pixel_image_print_truecolor(&img);

	ret = pixel_image_debayer(&img, window_size);
	zassert_ok(ret);

	pixel_image_to_buffer(&img, rgb24frame_out, sizeof(rgb24frame_out));

	printf("output: (expecting #%06x, R:%02x G:%02x B:%02x)\n", expected_color, r, g, b);
	pixel_image_print_truecolor(&img);

	for (int i = 0; i < sizeof(rgb24frame_out) / 3; i++) {
		uint8_t out_r = rgb24frame_out[i * 3 + 0];
		uint8_t out_g = rgb24frame_out[i * 3 + 1];
		uint8_t out_b = rgb24frame_out[i * 3 + 2];
		char *s = VIDEO_FOURCC_TO_STR(fourcc);

		zassert_equal(r, out_r, "R: %s: expected 0x%02x, obtained 0x%02x", s, r, out_r);
		zassert_equal(g, out_g, "G: %s: expected 0x%02x, obtained 0x%02x", s, g, out_g);
		zassert_equal(b, out_b, "B: %s: expected 0x%02x, obtained 0x%02x", s, b, out_b);
	}
}

ZTEST(lib_pixel_bayer, test_pixel_bayer_operation)
{
	/* Generate test input data for 2x2 debayer */
	for (size_t h = 0; h < HEIGHT; h++) {
		memset(bayerframe_in + h * WIDTH, h % 2 ? 0xff : 0x00, WIDTH * 1);
	}

	test_bayer(VIDEO_PIX_FMT_RGGB8, 2, 0x007fff);
	test_bayer(VIDEO_PIX_FMT_GRBG8, 2, 0x007fff);
	test_bayer(VIDEO_PIX_FMT_BGGR8, 2, 0xff7f00);
	test_bayer(VIDEO_PIX_FMT_GBRG8, 2, 0xff7f00);

	/* Generate test input data for 3x3 debayer */
	for (size_t h = 0; h < HEIGHT; h++) {
		for (size_t w = 0; w < WIDTH; w++) {
			bayerframe_in[h * WIDTH + w] = (h + w) % 2 ? 0xff : 0x00;
		}
	}

	test_bayer(VIDEO_PIX_FMT_RGGB8, 3, 0x00ff00);
	test_bayer(VIDEO_PIX_FMT_GBRG8, 3, 0xff00ff);
	test_bayer(VIDEO_PIX_FMT_BGGR8, 3, 0x00ff00);
	test_bayer(VIDEO_PIX_FMT_GRBG8, 3, 0xff00ff);
}

ZTEST_SUITE(lib_pixel_bayer, NULL, NULL, NULL, NULL, NULL);

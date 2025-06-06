/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/video.h>
#include <zephyr/pixel/convert.h>
#include <zephyr/pixel/print.h>
#include <zephyr/pixel/image.h>

#define WIDTH 16
#define HEIGHT 16

#define ERROR_MARGIN 13

/*
 * To get YUV BT.709 test data:
 *
 *	ffmpeg -y -f lavfi -colorspace bt709 -i color=#RRGGBB:2x2:d=3,format=rgb24 \
 *		-f rawvideo -pix_fmt yuyv422 - | hexdump -C
 *
 * To get RGB565 test data:
 *
 *	ffmpeg -y -f lavfi -i color=#RRGGBB:2x2:d=3,format=rgb24 \
 *		-f rawvideo -pix_fmt rgb565 - | hexdump -C
 */

const struct color_ref {
	uint8_t rgb24[3];
	uint8_t rgb565[2];
	uint8_t rgb332[1];
	uint8_t yuv24_bt709[3];
	uint8_t yuv24_bt601[3];
} reference_data[] = {

	/* Primary colors */
	{{0x00, 0x00, 0x00}, {0x00, 0x00}, {0x00}, {0x10, 0x80, 0x80}, {0x10, 0x80, 0x80}},
	{{0x00, 0x00, 0xff}, {0x00, 0x1f}, {0x03}, {0x20, 0xf0, 0x76}, {0x29, 0xf1, 0x6e}},
	{{0x00, 0xff, 0x00}, {0x07, 0xe0}, {0x1c}, {0xad, 0x2a, 0x1a}, {0x9a, 0x2a, 0x35}},
	{{0x00, 0xff, 0xff}, {0x07, 0xff}, {0x1f}, {0xbc, 0x9a, 0x10}, {0xb4, 0xa0, 0x23}},
	{{0xff, 0x00, 0x00}, {0xf8, 0x00}, {0xe0}, {0x3f, 0x66, 0xf0}, {0x50, 0x5b, 0xee}},
	{{0xff, 0x00, 0xff}, {0xf8, 0x1f}, {0xe3}, {0x4e, 0xd6, 0xe6}, {0x69, 0xcb, 0xdc}},
	{{0xff, 0xff, 0x00}, {0xff, 0xe0}, {0xfc}, {0xdb, 0x10, 0x8a}, {0xd0, 0x0a, 0x93}},
	{{0xff, 0xff, 0xff}, {0xff, 0xff}, {0xff}, {0xeb, 0x80, 0x80}, {0xeb, 0x80, 0x80}},

	/* Arbitrary colors */
	{{0x00, 0x70, 0xc5}, {0x03, 0x98}, {0x0f}, {0x61, 0xb1, 0x4b}, {0x5e, 0xb5, 0x4d}},
	{{0x33, 0x8d, 0xd1}, {0x3c, 0x7a}, {0x33}, {0x7d, 0xa7, 0x56}, {0x7b, 0xab, 0x57}},
	{{0x66, 0xa9, 0xdc}, {0x6d, 0x5b}, {0x77}, {0x98, 0x9d, 0x61}, {0x96, 0xa0, 0x61}},
	{{0x7d, 0xd2, 0xf7}, {0x86, 0x9e}, {0x7b}, {0xb7, 0x99, 0x59}, {0xb3, 0x9d, 0x5a}},
	{{0x97, 0xdb, 0xf9}, {0x9e, 0xde}, {0x9b}, {0xc2, 0x94, 0x61}, {0xbf, 0x97, 0x62}},
	{{0xb1, 0xe4, 0xfa}, {0xb7, 0x3f}, {0xbf}, {0xcc, 0x8f, 0x69}, {0xca, 0x91, 0x69}},
	{{0x79, 0x29, 0xd2}, {0x79, 0x5a}, {0x67}, {0x4c, 0xc2, 0x9c}, {0x57, 0xbf, 0x96}},
	{{0x94, 0x54, 0xdb}, {0x9a, 0xbb}, {0x8b}, {0x6c, 0xb5, 0x97}, {0x75, 0xb3, 0x92}},
	{{0xaf, 0x7f, 0xe4}, {0xb3, 0xfc}, {0xaf}, {0x8c, 0xa8, 0x91}, {0x93, 0xa6, 0x8d}},
};

static uint8_t line_in[WIDTH * 4];
static uint8_t line_out[WIDTH * 4];

void test_conversion(const uint8_t *pix_in, uint32_t fourcc_in, size_t pix_in_step,
		     const uint8_t *pix_out, uint32_t fourcc_out, size_t pix_out_step,
		     void (*fn)(const uint8_t *in, uint8_t *out, uint16_t width))
{
	size_t pix_in_size = video_bits_per_pixel(fourcc_in) / BITS_PER_BYTE;
	size_t pix_out_size = video_bits_per_pixel(fourcc_out) / BITS_PER_BYTE;
	bool done = false;

	/* Fill the input line as much as possible */
	for (size_t w = 0; w < WIDTH; w += pix_in_step) {
		memcpy(&line_in[w * pix_in_size], pix_in, pix_in_size * pix_in_step);
	}

	/* Perform the conversion to test */
	fn(line_in, line_out, WIDTH);

	printf("\n");

	printf("out:");
	for (int i = 0; i < pix_out_step * pix_out_size; i++) {
		printf(" %02x", line_out[i]);
	}
	printf(" |");
	pixel_print_buffer_truecolor(line_out, sizeof(line_out), WIDTH / 2, 2, fourcc_out);

	printf("ref:");
	for (int i = 0; i < pix_out_step * pix_out_size; i++) {
		printf(" %02x", pix_out[i]);
	}
	printf(" |");
	pixel_print_buffer_truecolor(line_in, sizeof(line_in), WIDTH / 2, 2, fourcc_in);

	/* Scan the result against the reference output pixel to make sure it worked */
	for (size_t w = 0; w < WIDTH; w += pix_out_step) {
		for (int i = 0; w * pix_out_size + i < (w + pix_out_step) * pix_out_size; i++) {
			zassert_within(line_out[w * pix_out_size + i], pix_out[i], 9,
				       "at %u: value 0x%02x, reference 0x%02x",
				       i, line_out[w * pix_out_size + i], pix_out[i]);
		}

		/* Make sure we visited that loop */
		done = true;
	}

	zassert_true(done);
}

ZTEST(lib_pixel_convert, test_pixel_convert_line)
{
	for (size_t i = 0; i < ARRAY_SIZE(reference_data); i++) {
		/* The current color we are testing */
		const struct color_ref *ref = &reference_data[i];

		/* Generate very small buffers out of the reference tables */
		const uint8_t rgb24[] = {
			ref->rgb24[0],
			ref->rgb24[1],
			ref->rgb24[2],
		};
		const uint8_t rgb565be[] = {
			ref->rgb565[0],
			ref->rgb565[1],
		};
		const uint8_t rgb565le[] = {
			ref->rgb565[1],
			ref->rgb565[0],
		};
		const uint8_t rgb332[] = {
			ref->rgb332[0],
		};
		const uint8_t yuv24_bt709[] = {
			ref->yuv24_bt709[0],
			ref->yuv24_bt709[1],
			ref->yuv24_bt709[2],
		};
		const uint8_t yuyv_bt709[] = {
			ref->yuv24_bt709[0],
			ref->yuv24_bt709[1],
			ref->yuv24_bt709[0],
			ref->yuv24_bt709[2],
		};

		printf("\nColor #%02x%02x%02x\n", ref->rgb24[0], ref->rgb24[1], ref->rgb24[2]);

		test_conversion(rgb24, VIDEO_PIX_FMT_RGB24, 1, rgb565be,
				VIDEO_PIX_FMT_RGB565X, 1, &pixel_line_rgb24_to_rgb565be);
		test_conversion(rgb24, VIDEO_PIX_FMT_RGB24, 1, rgb565le,
				VIDEO_PIX_FMT_RGB565, 1, &pixel_line_rgb24_to_rgb565le);
		test_conversion(rgb24, VIDEO_PIX_FMT_RGB24, 1, rgb332,
				VIDEO_PIX_FMT_RGB332, 1, &pixel_line_rgb24_to_rgb332);
		test_conversion(rgb565be, VIDEO_PIX_FMT_RGB565X, 1, rgb24,
				VIDEO_PIX_FMT_RGB24, 1, &pixel_line_rgb565be_to_rgb24);
		test_conversion(rgb565le, VIDEO_PIX_FMT_RGB565, 1, rgb24,
				VIDEO_PIX_FMT_RGB24, 1, &pixel_line_rgb565le_to_rgb24);
		test_conversion(rgb24, VIDEO_PIX_FMT_RGB24, 1, yuyv_bt709,
				VIDEO_PIX_FMT_YUYV, 2, &pixel_line_rgb24_to_yuyv_bt709);
		test_conversion(yuyv_bt709, VIDEO_PIX_FMT_YUYV, 2, rgb24,
				VIDEO_PIX_FMT_RGB24, 1, &pixel_line_yuyv_to_rgb24_bt709);
		test_conversion(rgb24, VIDEO_PIX_FMT_RGB24, 1, yuv24_bt709,
				VIDEO_PIX_FMT_YUV24, 1, &pixel_line_rgb24_to_yuv24_bt709);
		test_conversion(yuv24_bt709, VIDEO_PIX_FMT_YUV24, 1, rgb24,
				VIDEO_PIX_FMT_RGB24, 1, &pixel_line_yuv24_to_rgb24_bt709);
		test_conversion(yuv24_bt709, VIDEO_PIX_FMT_YUV24, 1, yuyv_bt709,
				VIDEO_PIX_FMT_YUYV, 2, &pixel_line_yuv24_to_yuyv);
		test_conversion(yuyv_bt709, VIDEO_PIX_FMT_YUYV, 2, yuv24_bt709,
				VIDEO_PIX_FMT_YUV24, 1, &pixel_line_yuyv_to_yuv24);
	}
}

static uint8_t rgb24frame_in[WIDTH * HEIGHT * 3];
static uint8_t rgb24frame_out[WIDTH * HEIGHT * 3];

ZTEST(lib_pixel_convert, test_pixel_convert_operation)
{
	struct pixel_image img;
	int ret;

	/* Generate test input data */
	for (size_t i = 0; i < sizeof(rgb24frame_in); i++) {
		rgb24frame_in[i] = i / 3;
	}

	pixel_image_from_buffer(&img, rgb24frame_in, sizeof(rgb24frame_in),
				WIDTH, HEIGHT, VIDEO_PIX_FMT_RGB24);

	printf("input:\n");
	pixel_image_print_truecolor(&img);

	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_RGB24);
	zassert_ok(ret);

	/* Test the RGB24 <-> RGB565 conversion */
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_RGB565);
	zassert_ok(ret);
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_RGB24);
	zassert_ok(ret);

	/* Test the RGB24 <-> RGB565X conversion */
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_RGB565X);
	zassert_ok(ret);
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_RGB24);
	zassert_ok(ret);

	/* Test the RGB24 <-> YUV24 conversion */
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_YUV24);
	zassert_ok(ret);
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_RGB24);
	zassert_ok(ret);

	/* Test the YUYV <-> YUV24 conversion */
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_YUYV);
	zassert_ok(ret);
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_YUV24);
	zassert_ok(ret);
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_YUYV);
	zassert_ok(ret);
	ret = pixel_image_convert(&img, VIDEO_PIX_FMT_RGB24);
	zassert_ok(ret);

	pixel_image_to_buffer(&img, rgb24frame_out, sizeof(rgb24frame_out));

	printf("output:\n");
	pixel_image_print_truecolor(&img);

	for (int i = 0; i < sizeof(rgb24frame_out); i++) {
		/* Precision is not 100% as some conversions steps are lossy */
		zassert_within(rgb24frame_in[i], rgb24frame_out[i], ERROR_MARGIN,
			       "Testing position %u", i);
	}
}

ZTEST_SUITE(lib_pixel_convert, NULL, NULL, NULL, NULL, NULL);

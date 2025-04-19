/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/pixel/formats.h>
#include <zephyr/pixel/print.h>

#define WIDTH 16
#define HEIGHT 16

#define ERROR_MARGIN 10

/*
 * To get YUV BT.709 test data:
 *
 *	ffmpeg -y -f lavfi -colorspace bt709 -i color=#rrggbb:2x2:d=3,format=rgb24 \
 *		-f rawvideo -pix_fmt yuyv422 - | hexdump -C
 *
 * To get RGB565 test data:
 *
 *	ffmpeg -y -f lavfi -i color=$rgb:2x2:d=3,format=rgb24 \
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

void test_conversion(const uint8_t *pix_in, size_t pix_in_size, size_t pix_in_step,
		     const uint8_t *pix_out, size_t pix_out_size, size_t pix_out_step,
		     void (*fn)(const uint8_t *in, uint8_t *out, uint16_t width))
{
	bool done = false;

	/* Fill the input line as much as possible */
	for (size_t w = 0; w < WIDTH; w += pix_in_step) {
		memcpy(&line_in[w * pix_in_size], pix_in, pix_in_size * pix_in_step);
	}

	/* Perform the conversion to test */
	fn(line_in, line_out, WIDTH);

	printf("out:");
	for (int i = 0; i < pix_out_step * pix_out_size; i++) {
		printf(" %02x", line_out[i]);
	}
	printf("\n");
	printf("ref:");
	for (int i = 0; i < pix_out_step * pix_out_size; i++) {
		printf(" %02x", pix_out[i]);
	}
	printf("\n");

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

ZTEST(lib_pixel_format, test_pixel_format_line)
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
		const uint8_t yuyv_bt709[] = {
			ref->yuv24_bt709[0],
			ref->yuv24_bt709[1],
			ref->yuv24_bt709[0],
			ref->yuv24_bt709[2],
		};

		printf("\nColor #%02x%02x%02x\n", ref->rgb24[0], ref->rgb24[1], ref->rgb24[2]);

		printf("RGB24 in, RGB565BE out\n");
		test_conversion(rgb24, 3, 1, rgb565be, 2, 1, &pixel_rgb24line_to_rgb565beline);
		pixel_print_rgb24frame_truecolor(line_in, sizeof(line_in), WIDTH / 2, 2);
		pixel_print_rgb565beframe_truecolor(line_out, sizeof(line_out), WIDTH / 2, 2);

		printf("RGB24 in, RGB565LE out\n");
		test_conversion(rgb24, 3, 1, rgb565le, 2, 1, &pixel_rgb24line_to_rgb565leline);
		pixel_print_rgb24frame_truecolor(line_in, sizeof(line_in), WIDTH / 2, 2);
		pixel_print_rgb565leframe_truecolor(line_out, sizeof(line_out), WIDTH / 2, 2);

		printf("RGB24 in, RGB332 out\n");
		test_conversion(rgb24, 3, 1, rgb332, 1, 1, &pixel_rgb24line_to_rgb332line);
		pixel_print_rgb24frame_truecolor(line_in, sizeof(line_in), WIDTH / 2, 2);
		pixel_print_rgb332frame_truecolor(line_out, sizeof(line_out), WIDTH / 2, 2);

		printf("RGB565BE in, RGB24 out\n");
		test_conversion(rgb565be, 2, 1, rgb24, 3, 1, &pixel_rgb565beline_to_rgb24line);
		pixel_print_rgb565beframe_truecolor(line_in, sizeof(line_in), WIDTH / 2, 2);
		pixel_print_rgb24frame_truecolor(line_out, sizeof(line_out), WIDTH / 2, 2);

		printf("RGB565LE in, RGB24 out\n");
		test_conversion(rgb565le, 2, 1, rgb24, 3, 1, &pixel_rgb565leline_to_rgb24line);
		pixel_print_rgb565leframe_truecolor(line_in, sizeof(line_in), WIDTH / 2, 2);
		pixel_print_rgb24frame_truecolor(line_out, sizeof(line_out), WIDTH / 2, 2);

		printf("RGB24 in, YUYV (BT.709) out\n");
		test_conversion(rgb24, 3, 1, yuyv_bt709, 2, 2, &pixel_rgb24line_to_yuyvline_bt709);
		pixel_print_rgb24frame_truecolor(line_in, sizeof(line_in), WIDTH / 2, 2);
		pixel_print_yuyvframe_bt709_truecolor(line_out, sizeof(line_out), WIDTH / 2, 2);

		printf("YUYV (BT.709) in, RGB24 out\n");
		test_conversion(yuyv_bt709, 2, 2, rgb24, 3, 1, &pixel_yuyvline_to_rgb24line_bt709);
		pixel_print_yuyvframe_bt709_truecolor(line_in, sizeof(line_in), WIDTH / 2, 2);
		pixel_print_rgb24frame_truecolor(line_out, sizeof(line_out), WIDTH / 2, 2);
	}
}

/* From RGB24 */
static PIXEL_RGB24STREAM_TO_RGB565BESTREAM(step_rgb24_to_rgb565be, WIDTH, HEIGHT);
static PIXEL_RGB24STREAM_TO_RGB565LESTREAM(step_rgb24_to_rgb565le, WIDTH, HEIGHT);
static PIXEL_RGB24STREAM_TO_YUYVSTREAM_BT709(step_rgb24_to_yuyv, WIDTH, HEIGHT);

/* To RGB24 */
static PIXEL_RGB565BESTREAM_TO_RGB24STREAM(step_rgb565be_to_rgb24, WIDTH, HEIGHT);
static PIXEL_RGB565LESTREAM_TO_RGB24STREAM(step_rgb565le_to_rgb24, WIDTH, HEIGHT);
static PIXEL_YUYVSTREAM_TO_RGB24STREAM_BT709(step_yuyv_to_rgb24, WIDTH, HEIGHT);

static uint8_t rgb24frame_in[WIDTH * HEIGHT * 3];
static uint8_t rgb24frame_out[WIDTH * HEIGHT * 3];

ZTEST(lib_pixel_format, test_pixel_format_stream)
{
	/* Generate test input data */
	for (size_t i = 0; i < sizeof(rgb24frame_in); i++) {
		rgb24frame_in[i] = i / 3;
	}

	/* Run a pipeline going through all possible steps */
	pixel_stream_to_rgb24frame(rgb24frame_in, sizeof(rgb24frame_in), WIDTH,
				   rgb24frame_out, sizeof(rgb24frame_out), WIDTH,
				   &step_rgb24_to_rgb565le, &step_rgb565le_to_rgb24,
				   &step_rgb24_to_rgb565be, &step_rgb565be_to_rgb24,
				   &step_rgb24_to_yuyv, &step_yuyv_to_rgb24, NULL);

	printf("input:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_in, sizeof(rgb24frame_in), WIDTH, HEIGHT);

	printf("output:\n");
	pixel_print_rgb24frame_truecolor(rgb24frame_out, sizeof(rgb24frame_out), WIDTH, HEIGHT);

	for (int i = 0; i < sizeof(rgb24frame_out); i++) {
		/* Precision is not 100% as some conversions steps are lossy */
		zassert_within(rgb24frame_in[i], rgb24frame_out[i], ERROR_MARGIN,
			       "Testing position %u", i);
	}
}

ZTEST_SUITE(lib_pixel_format, NULL, NULL, NULL, NULL, NULL);

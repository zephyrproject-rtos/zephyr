/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include <zephyr_jpegenc.h>

/* Neither dimension is a multiple of the 16-pixel MCU */
#define WIDTH         70
#define HEIGHT        50
#define CHROMA_WIDTH  ((WIDTH + 1) / 2)
#define CHROMA_HEIGHT ((HEIGHT + 1) / 2)

static struct zephyr_jpegenc enc;
static uint8_t out_a[32768];
static uint8_t out_b[32768];

static void check_markers(const uint8_t *jpeg, size_t size)
{
	zassert_true(size > 4U, "%zu-byte output", size);
	zassert_true((jpeg[0] == 0xffU) && (jpeg[1] == 0xd8U), "no start-of-image marker");
	zassert_true((jpeg[size - 2] == 0xffU) && (jpeg[size - 1] == 0xd9U),
		     "no end-of-image marker");
}

static size_t encode_packed(enum zephyr_jpegenc_format format, const void *pixels, size_t pitch,
			    enum zephyr_jpegenc_subsampling subsampling, uint8_t *out,
			    size_t out_size)
{
	size_t size = 0U;
	int ret;

	ret = zephyr_jpegenc_begin(&enc, out, out_size, WIDTH, HEIGHT, format, subsampling,
				   ZEPHYR_JPEGENC_QUALITY_HIGH);
	zassert_equal(ret, 0, "begin failed: %d", ret);
	ret = zephyr_jpegenc_add_frame(&enc, pixels, pitch);
	zassert_equal(ret, 0, "add frame failed: %d", ret);
	ret = zephyr_jpegenc_end(&enc, &size);
	zassert_equal(ret, 0, "end failed: %d", ret);
	check_markers(out, size);

	return size;
}

static uint16_t gradient(int x, int y)
{
	return (uint16_t)(((x * 31 / WIDTH) << 11) | ((y * 63 / HEIGHT) << 5) | ((x + y) & 31));
}

ZTEST(jpegenc, test_rgb565_byte_order)
{
	static uint16_t le[WIDTH * HEIGHT];
	static uint16_t be[WIDTH * HEIGHT];
	static const enum zephyr_jpegenc_subsampling subsamplings[] = {
		ZEPHYR_JPEGENC_SUBSAMPLING_444,
		ZEPHYR_JPEGENC_SUBSAMPLING_420,
	};

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			le[y * WIDTH + x] = sys_cpu_to_le16(gradient(x, y));
			be[y * WIDTH + x] = sys_cpu_to_be16(gradient(x, y));
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(subsamplings); i++) {
		size_t size_le = encode_packed(ZEPHYR_JPEGENC_RGB565_LE, le, WIDTH * 2,
					       subsamplings[i], out_a, sizeof(out_a));
		size_t size_be = encode_packed(ZEPHYR_JPEGENC_RGB565_BE, be, WIDTH * 2,
					       subsamplings[i], out_b, sizeof(out_b));

		zassert_equal(size_le, size_be, "LE and BE output sizes differ");
		zassert_mem_equal(out_a, out_b, size_le, "LE and BE outputs differ");
	}
}

ZTEST(jpegenc, test_yuv420p_nv12)
{
	static uint8_t y[WIDTH * HEIGHT];
	static uint8_t u[CHROMA_WIDTH * CHROMA_HEIGHT];
	static uint8_t v[CHROMA_WIDTH * CHROMA_HEIGHT];
	static uint8_t uv[2 * CHROMA_WIDTH * CHROMA_HEIGHT];
	size_t size_420p = 0U;
	size_t size_nv12 = 0U;

	for (size_t i = 0; i < ARRAY_SIZE(y); i++) {
		y[i] = (uint8_t)(i * 7U);
	}
	for (size_t i = 0; i < ARRAY_SIZE(u); i++) {
		u[i] = (uint8_t)(64U + i * 3U);
		v[i] = (uint8_t)(192U - i * 5U);
		uv[2 * i] = u[i];
		uv[2 * i + 1] = v[i];
	}

	zassert_equal(zephyr_jpegenc_begin(&enc, out_a, sizeof(out_a), WIDTH, HEIGHT,
					   ZEPHYR_JPEGENC_YUV420P, ZEPHYR_JPEGENC_SUBSAMPLING_420,
					   ZEPHYR_JPEGENC_QUALITY_HIGH),
		      0);
	zassert_equal(
		zephyr_jpegenc_add_frame_yuv420p(&enc, y, WIDTH, u, CHROMA_WIDTH, v, CHROMA_WIDTH),
		0);
	zassert_equal(zephyr_jpegenc_end(&enc, &size_420p), 0);
	check_markers(out_a, size_420p);

	zassert_equal(zephyr_jpegenc_begin(&enc, out_b, sizeof(out_b), WIDTH, HEIGHT,
					   ZEPHYR_JPEGENC_NV12, ZEPHYR_JPEGENC_SUBSAMPLING_420,
					   ZEPHYR_JPEGENC_QUALITY_HIGH),
		      0);
	zassert_equal(zephyr_jpegenc_add_frame_nv12(&enc, y, WIDTH, uv, 2 * CHROMA_WIDTH), 0);
	zassert_equal(zephyr_jpegenc_end(&enc, &size_nv12), 0);

	zassert_equal(size_420p, size_nv12, "YUV 4:2:0 and NV12 output sizes differ");
	zassert_mem_equal(out_a, out_b, size_420p, "YUV 4:2:0 and NV12 outputs differ");
}

ZTEST(jpegenc, test_output_full)
{
	static uint8_t noise[128 * 128];
	uint32_t state = 1U;

	for (size_t i = 0; i < ARRAY_SIZE(noise); i++) {
		state = state * 1103515245U + 12345U;
		noise[i] = (uint8_t)(state >> 16);
	}

	zassert_equal(zephyr_jpegenc_begin(&enc, out_a, ZEPHYR_JPEGENC_MIN_OUTPUT_SIZE, 128, 128,
					   ZEPHYR_JPEGENC_GRAY8, ZEPHYR_JPEGENC_SUBSAMPLING_444,
					   ZEPHYR_JPEGENC_QUALITY_BEST),
		      0);
	zassert_equal(zephyr_jpegenc_add_frame(&enc, noise, 128), -ENOSPC);
}

ZTEST(jpegenc, test_invalid)
{
	static uint8_t gray[WIDTH * HEIGHT];
	size_t size;

	zassert_equal(zephyr_jpegenc_begin(&enc, out_a, ZEPHYR_JPEGENC_MIN_OUTPUT_SIZE - 1U, WIDTH,
					   HEIGHT, ZEPHYR_JPEGENC_GRAY8,
					   ZEPHYR_JPEGENC_SUBSAMPLING_444,
					   ZEPHYR_JPEGENC_QUALITY_HIGH),
		      -EINVAL);
	zassert_equal(zephyr_jpegenc_begin(&enc, out_a, sizeof(out_a), WIDTH, HEIGHT,
					   ZEPHYR_JPEGENC_YUYV, ZEPHYR_JPEGENC_SUBSAMPLING_444,
					   ZEPHYR_JPEGENC_QUALITY_HIGH),
		      -ENOTSUP);
	zassert_equal(zephyr_jpegenc_end(&enc, &size), -EINVAL, "finished an image never begun");

	/* Planar input needs its own add-frame call */
	zassert_equal(zephyr_jpegenc_begin(&enc, out_a, sizeof(out_a), WIDTH, HEIGHT,
					   ZEPHYR_JPEGENC_YUV420P, ZEPHYR_JPEGENC_SUBSAMPLING_420,
					   ZEPHYR_JPEGENC_QUALITY_HIGH),
		      0);
	zassert_equal(zephyr_jpegenc_add_frame(&enc, gray, WIDTH), -EINVAL);
}

ZTEST_SUITE(jpegenc, NULL, NULL, NULL, NULL, NULL);

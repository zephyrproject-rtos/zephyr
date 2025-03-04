/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/pixel/stats.h>
#include <zephyr/pixel/formats.h>

#define PIXEL_IDX_R 0
#define PIXEL_IDX_G 1
#define PIXEL_IDX_B 2

static const uint8_t pixel_idx_rggb8[4] = {PIXEL_IDX_R, PIXEL_IDX_G, PIXEL_IDX_G, PIXEL_IDX_B};
static const uint8_t pixel_idx_bggr8[4] = {PIXEL_IDX_B, PIXEL_IDX_G, PIXEL_IDX_G, PIXEL_IDX_R};
static const uint8_t pixel_idx_gbrg8[4] = {PIXEL_IDX_G, PIXEL_IDX_B, PIXEL_IDX_R, PIXEL_IDX_G};
static const uint8_t pixel_idx_grbg8[4] = {PIXEL_IDX_G, PIXEL_IDX_R, PIXEL_IDX_B, PIXEL_IDX_G};

/* Extract a random value from the buffer */

static inline uint32_t pixel_rand(void)
{
	static uint32_t lcg_state;

	/* Linear Congruent Generator (LCG) are low-quality but very fast, here considered enough
	 * as even a fixed offset would have been enough.The % phase is skipped as there is already
	 * "% vbuf->bytesused" downstream in the code.
	 *
	 * The constants are from https://en.wikipedia.org/wiki/Linear_congruential_generator
	 */
	lcg_state = lcg_state * 1103515245 + 12345;
	return lcg_state;
}

static inline void pixel_sample_rgb24(const uint8_t *buf, size_t size, uint8_t rgb24[3])
{
	uint32_t pos = pixel_rand() % size;

	/* Align on 24-bit pixel boundary */
	pos -= pos % 3;

	rgb24[0] = buf[pos + 0];
	rgb24[1] = buf[pos + 1];
	rgb24[2] = buf[pos + 2];
}

static inline void pixel_sample_bayer(const uint8_t *buf, size_t size, uint16_t width,
				      uint8_t rgb24[3], const uint8_t *idx)
{
	uint32_t pos = pixel_rand() % size;

	/* Make sure to be on even row and column position */
	pos -= pos % 2;
	pos -= pos / width % 2 * width;

	rgb24[idx[0]] = buf[pos + 0];
	rgb24[idx[1]] = buf[pos + 1];
	rgb24[idx[2]] = buf[pos + width + 0];
	rgb24[idx[3]] = buf[pos + width + 1];
}

static inline void pixel_sums_to_rgb24avg(uint32_t sums[3], uint8_t rgb24avg[3], uint16_t nval)
{
	rgb24avg[0] = sums[0] / nval;
	rgb24avg[1] = sums[1] / nval;
	rgb24avg[2] = sums[2] / nval;
}

static inline void pixel_sums_add_rgb24(uint32_t sums[3], uint8_t rgb24[3])
{
	sums[0] += rgb24[0], sums[1] += rgb24[1];
	sums[2] += rgb24[2];
}

/* Channel average statistics */

void pixel_rgb24frame_to_rgb24avg(const uint8_t *buf, size_t size, uint8_t rgb24avg[3],
				  uint16_t nval)
{
	uint32_t sums[3] = {0, 0, 0};
	uint8_t rgb24[3];

	for (uint16_t n = 0; n < nval; n++) {
		pixel_sample_rgb24(buf, size, rgb24);
		pixel_sums_add_rgb24(sums, rgb24);
	}
	pixel_sums_to_rgb24avg(sums, rgb24avg, nval);
}

static inline void pixel_bayerframe_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
						uint8_t rgb24avg[3], uint16_t nval,
						const uint8_t *idx)
{
	uint32_t sums[3] = {0, 0, 0};
	uint8_t rgb24[3];

	for (uint16_t n = 0; n < nval; n++) {
		pixel_sample_bayer(buf, size, width, rgb24, idx);
		pixel_sums_add_rgb24(sums, rgb24);
	}
	pixel_sums_to_rgb24avg(sums, rgb24avg, nval);
}

void pixel_rggb8frame_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
				  uint8_t rgb24avg[3], uint16_t nval)
{
	pixel_bayerframe_to_rgb24avg(buf, size, width, rgb24avg, nval, pixel_idx_rggb8);
}

void pixel_bggr8frame_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
				  uint8_t rgb24avg[3], uint16_t nval)
{
	pixel_bayerframe_to_rgb24avg(buf, size, width, rgb24avg, nval, pixel_idx_bggr8);
}

void pixel_gbrg8frame_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
				  uint8_t rgb24avg[3], uint16_t nval)
{
	pixel_bayerframe_to_rgb24avg(buf, size, width, rgb24avg, nval, pixel_idx_gbrg8);
}

void pixel_grbg8frame_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
				  uint8_t rgb24avg[3], uint16_t nval)
{
	pixel_bayerframe_to_rgb24avg(buf, size, width, rgb24avg, nval, pixel_idx_grbg8);
}

/* RGB24 histogram statistics */

static inline void pixel_rgb24hist_add_rgb24(uint16_t *rgb24hist, uint8_t rgb24[3],
					     uint8_t bit_depth)
{
	uint16_t *r8hist = &rgb24hist[0 * (1 << bit_depth)], r8 = rgb24[0];
	uint16_t *g8hist = &rgb24hist[1 * (1 << bit_depth)], g8 = rgb24[1];
	uint16_t *b8hist = &rgb24hist[2 * (1 << bit_depth)], b8 = rgb24[2];

	r8hist[r8 >> (BITS_PER_BYTE - bit_depth)]++;
	g8hist[g8 >> (BITS_PER_BYTE - bit_depth)]++;
	b8hist[b8 >> (BITS_PER_BYTE - bit_depth)]++;
}

void pixel_rgb24frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t *rgb24hist,
				   size_t hist_size, uint16_t nval)
{
	uint8_t bit_depth = LOG2(hist_size / 3);
	uint8_t rgb24[3];

	__ASSERT(hist_size % 3 == 0, "Each of R, G, B channel should have the same size.");
	__ASSERT(1 << bit_depth == hist_size / 3, "Each channel size should be a power of two.");

	memset(rgb24hist, 0x00, hist_size * sizeof(*rgb24hist));

	for (uint16_t n = 0; n < nval; n++) {
		pixel_sample_rgb24(buf, buf_size, rgb24);
		pixel_rgb24hist_add_rgb24(rgb24hist, rgb24, bit_depth);
	}
}

static inline void pixel_bayerframe_to_rgb24hist(const uint8_t *buf, size_t buf_size,
						 uint16_t width, uint16_t *rgb24hist,
						 size_t hist_size, uint16_t nval,
						 const uint8_t *idx)
{
	uint8_t bit_depth = LOG2(hist_size / 3);
	uint8_t rgb24[3];

	__ASSERT(hist_size % 3 == 0, "Each of R, G, B channel should have the same size.");
	__ASSERT(1 << bit_depth == hist_size / 3, "Each channel size should be a power of two.");

	memset(rgb24hist, 0x00, hist_size * sizeof(*rgb24hist));

	for (uint16_t n = 0; n < nval; n++) {
		pixel_sample_bayer(buf, buf_size, width, rgb24, idx);
		pixel_rgb24hist_add_rgb24(rgb24hist, rgb24, bit_depth);
	}
}

void pixel_rggb8frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				   uint16_t *rgb24hist, size_t hist_size, uint16_t nval)
{
	pixel_bayerframe_to_rgb24hist(buf, buf_size, width, rgb24hist, hist_size, nval,
				      pixel_idx_rggb8);
}

void pixel_gbrg8frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				   uint16_t *rgb24hist, size_t hist_size, uint16_t nval)
{
	pixel_bayerframe_to_rgb24hist(buf, buf_size, width, rgb24hist, hist_size, nval,
				      pixel_idx_gbrg8);
}

void pixel_bggr8frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				   uint16_t *rgb24hist, size_t hist_size, uint16_t nval)
{
	pixel_bayerframe_to_rgb24hist(buf, buf_size, width, rgb24hist, hist_size, nval,
				      pixel_idx_bggr8);
}

void pixel_grbg8frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				   uint16_t *rgb24hist, size_t hist_size, uint16_t nval)
{
	pixel_bayerframe_to_rgb24hist(buf, buf_size, width, rgb24hist, hist_size, nval,
				      pixel_idx_grbg8);
}

/* Y8 histogram statistics
 * Use BT.709 (sRGB) as an arbitrary choice, instead of BT.601 like libcamera
 */

static inline void pixel_y8hist_add_y8(uint16_t *y8hist, uint8_t y8, uint8_t bit_depth)
{
	y8hist[y8 >> (BITS_PER_BYTE - bit_depth)]++;
}

void pixel_rgb24frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t *y8hist,
				size_t hist_size, uint16_t nval)
{
	uint8_t bit_depth = LOG2(hist_size);
	uint8_t rgb24[3];

	__ASSERT(1 << bit_depth == hist_size, "Histogram channel size should be a power of two.");

	memset(y8hist, 0x00, hist_size * sizeof(*y8hist));

	for (uint16_t n = 0; n < nval; n++) {
		pixel_sample_rgb24(buf, buf_size, rgb24);
		pixel_y8hist_add_y8(y8hist, pixel_rgb24_get_luma_bt709(rgb24), bit_depth);
	}
}

static inline void pixel_bayerframe_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
					      uint16_t *y8hist, size_t hist_size, uint16_t nval,
					      const uint8_t *idx)
{
	uint8_t bit_depth = LOG2(hist_size);
	uint8_t rgb24[3];

	__ASSERT(1 << bit_depth == hist_size, "Histogram channel size should be a power of two.");

	memset(y8hist, 0x00, hist_size * sizeof(*y8hist));

	for (uint16_t n = 0; n < nval; n++) {
		pixel_sample_bayer(buf, buf_size, width, rgb24, idx);
		pixel_y8hist_add_y8(y8hist, pixel_rgb24_get_luma_bt709(rgb24), bit_depth);
	}
}

void pixel_rggb8frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				uint16_t *y8hist, size_t hist_size, uint16_t nval)
{
	pixel_bayerframe_to_y8hist(buf, buf_size, width, y8hist, hist_size, nval, pixel_idx_rggb8);
}

void pixel_gbrg8frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				uint16_t *y8hist, size_t hist_size, uint16_t nval)
{
	pixel_bayerframe_to_y8hist(buf, buf_size, width, y8hist, hist_size, nval, pixel_idx_gbrg8);
}

void pixel_bggr8frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				uint16_t *y8hist, size_t hist_size, uint16_t nval)
{
	pixel_bayerframe_to_y8hist(buf, buf_size, width, y8hist, hist_size, nval, pixel_idx_bggr8);
}

void pixel_grbg8frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				uint16_t *y8hist, size_t hist_size, uint16_t nval)
{
	pixel_bayerframe_to_y8hist(buf, buf_size, width, y8hist, hist_size, nval, pixel_idx_grbg8);
}

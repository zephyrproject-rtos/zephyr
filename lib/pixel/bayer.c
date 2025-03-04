/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/pixel/bayer.h>

#define FOLD_L_3X3(l0, l1, l2)                                                                     \
	{                                                                                          \
		{l0[1], l0[0], l0[1]},                                                             \
		{l1[1], l1[0], l1[1]},                                                             \
		{l2[1], l2[0], l2[1]},                                                             \
	}

#define FOLD_R_3X3(l0, l1, l2, n)                                                                  \
	{                                                                                          \
		{l0[(n) - 2], l0[(n) - 1], l0[(n) - 2]},                                           \
		{l1[(n) - 2], l1[(n) - 1], l1[(n) - 2]},                                           \
		{l2[(n) - 2], l2[(n) - 1], l2[(n) - 2]},                                           \
	}

static inline void pixel_rggb8_to_rgb24_3x3(const uint8_t rgr0[3], const uint8_t gbg1[3],
					    const uint8_t rgr2[3], uint8_t rgb24[3])
{
	rgb24[0] = ((uint16_t)rgr0[0] + rgr0[2] + rgr2[0] + rgr2[2]) / 4;
	rgb24[1] = ((uint16_t)rgr0[1] + gbg1[2] + gbg1[0] + rgr2[1]) / 4;
	rgb24[2] = gbg1[1];
}

static inline void pixel_bggr8_to_rgb24_3x3(const uint8_t bgb0[3], const uint8_t grg1[3],
					    const uint8_t bgb2[3], uint8_t rgb24[3])
{
	rgb24[0] = grg1[1];
	rgb24[1] = ((uint16_t)bgb0[1] + grg1[2] + grg1[0] + bgb2[1]) / 4;
	rgb24[2] = ((uint16_t)bgb0[0] + bgb0[2] + bgb2[0] + bgb2[2]) / 4;
}

static inline void pixel_grbg8_to_rgb24_3x3(const uint8_t grg0[3], const uint8_t bgb1[3],
					    const uint8_t grg2[3], uint8_t rgb24[3])
{
	rgb24[0] = ((uint16_t)grg0[1] + grg2[1]) / 2;
	rgb24[1] = bgb1[1];
	rgb24[2] = ((uint16_t)bgb1[0] + bgb1[2]) / 2;
}

static inline void pixel_gbrg8_to_rgb24_3x3(const uint8_t gbg0[3], const uint8_t rgr1[3],
					    const uint8_t gbg2[3], uint8_t rgb24[3])
{
	rgb24[0] = ((uint16_t)rgr1[0] + rgr1[2]) / 2;
	rgb24[1] = rgr1[1];
	rgb24[2] = ((uint16_t)gbg0[1] + gbg2[1]) / 2;
}

static inline void pixel_rggb8_to_rgb24_2x2(uint8_t r0, uint8_t g0, uint8_t g1, uint8_t b0,
					    uint8_t rgb24[3])
{
	rgb24[0] = r0;
	rgb24[1] = ((uint16_t)g0 + g1) / 2;
	rgb24[2] = b0;
}

static inline void pixel_gbrg8_to_rgb24_2x2(uint8_t g1, uint8_t b0, uint8_t r0, uint8_t g0,
					    uint8_t rgb24[3])
{
	rgb24[0] = r0;
	rgb24[1] = ((uint16_t)g0 + g1) / 2;
	rgb24[2] = b0;
}

static inline void pixel_bggr8_to_rgb24_2x2(uint8_t b0, uint8_t g0, uint8_t g1, uint8_t r0,
					    uint8_t rgb24[3])
{
	rgb24[0] = r0;
	rgb24[1] = ((uint16_t)g0 + g1) / 2;
	rgb24[2] = b0;
}

static inline void pixel_grbg8_to_rgb24_2x2(uint8_t g1, uint8_t r0, uint8_t b0, uint8_t g0,
					    uint8_t rgb24[3])
{
	rgb24[0] = r0;
	rgb24[1] = ((uint16_t)g0 + g1) / 2;
	rgb24[2] = b0;
}

__weak void pixel_rggb8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1,
					     const uint8_t *i2, uint8_t *o0, uint16_t w)
{
	__ASSERT_NO_MSG(w >= 4 && w % 2 == 0);
	uint8_t il[3][3] = FOLD_L_3X3(i0, i1, i2);
	uint8_t ir[3][3] = FOLD_R_3X3(i0, i1, i2, w);

	pixel_grbg8_to_rgb24_3x3(il[0], il[1], il[2], &o0[0]);
	for (size_t i = 0, o = 3; i + 4 <= w; i += 2, o += 6) {
		pixel_rggb8_to_rgb24_3x3(&i0[i + 0], &i1[i + 0], &i2[i + 0], &o0[o + 0]);
		pixel_grbg8_to_rgb24_3x3(&i0[i + 1], &i1[i + 1], &i2[i + 1], &o0[o + 3]);
	}
	pixel_rggb8_to_rgb24_3x3(ir[0], ir[1], ir[2], &o0[w * 3 - 3]);
}

__weak void pixel_grbg8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1,
					     const uint8_t *i2, uint8_t *o0, uint16_t w)
{
	__ASSERT_NO_MSG(w >= 4 && w % 2 == 0);
	uint8_t il[3][4] = FOLD_L_3X3(i0, i1, i2);
	uint8_t ir[3][4] = FOLD_R_3X3(i0, i1, i2, w);

	pixel_rggb8_to_rgb24_3x3(il[0], il[1], il[2], &o0[0]);
	for (size_t i = 0, o = 3; i + 4 <= w; i += 2, o += 6) {
		pixel_grbg8_to_rgb24_3x3(&i0[i + 0], &i1[i + 0], &i2[i + 0], &o0[o + 0]);
		pixel_rggb8_to_rgb24_3x3(&i0[i + 1], &i1[i + 1], &i2[i + 1], &o0[o + 3]);
	}
	pixel_grbg8_to_rgb24_3x3(ir[0], ir[1], ir[2], &o0[w * 3 - 3]);
}

__weak void pixel_bggr8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1,
					     const uint8_t *i2, uint8_t *o0, uint16_t w)
{
	__ASSERT_NO_MSG(w >= 4 && w % 2 == 0);
	uint8_t il[3][4] = FOLD_L_3X3(i0, i1, i2);
	uint8_t ir[3][4] = FOLD_R_3X3(i0, i1, i2, w);

	pixel_gbrg8_to_rgb24_3x3(il[0], il[1], il[2], &o0[0]);
	for (size_t i = 0, o = 3; i + 4 <= w; i += 2, o += 6) {
		pixel_bggr8_to_rgb24_3x3(&i0[i + 0], &i1[i + 0], &i2[i + 0], &o0[o + 0]);
		pixel_gbrg8_to_rgb24_3x3(&i0[i + 1], &i1[i + 1], &i2[i + 1], &o0[o + 3]);
	}
	pixel_bggr8_to_rgb24_3x3(ir[0], ir[1], ir[2], &o0[w * 3 - 3]);
}

__weak void pixel_gbrg8line_to_rgb24line_3x3(const uint8_t *i0, const uint8_t *i1,
					     const uint8_t *i2, uint8_t *o0, uint16_t w)
{
	__ASSERT_NO_MSG(w >= 4 && w % 2 == 0);
	uint8_t il[3][4] = FOLD_L_3X3(i0, i1, i2);
	uint8_t ir[3][4] = FOLD_R_3X3(i0, i1, i2, w);

	pixel_bggr8_to_rgb24_3x3(il[0], il[1], il[2], &o0[0]);
	for (size_t i = 0, o = 3; i + 4 <= w; i += 2, o += 6) {
		pixel_gbrg8_to_rgb24_3x3(&i0[i + 0], &i1[i + 0], &i2[i + 0], &o0[o + 0]);
		pixel_bggr8_to_rgb24_3x3(&i0[i + 1], &i1[i + 1], &i2[i + 1], &o0[o + 3]);
	}
	pixel_gbrg8_to_rgb24_3x3(ir[0], ir[1], ir[2], &o0[w * 3 - 3]);
}

__weak void pixel_rggb8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *o0,
					     uint16_t w)
{
	__ASSERT_NO_MSG(w >= 2 && w % 2 == 0);

	for (size_t i = 0, o = 0; i + 3 <= w; i += 2, o += 6) {
		pixel_rggb8_to_rgb24_2x2(i0[i + 0], i0[i + 1], i1[i + 0], i1[i + 1], &o0[o + 0]);
		pixel_grbg8_to_rgb24_2x2(i0[i + 1], i0[i + 2], i1[i + 1], i1[i + 2], &o0[o + 3]);
	}
	pixel_rggb8_to_rgb24_2x2(i0[w - 1], i0[w - 2], i1[w - 1], i1[w - 2], &o0[w * 3 - 6]);
	pixel_grbg8_to_rgb24_2x2(i0[w - 2], i0[w - 1], i1[w - 2], i1[w - 1], &o0[w * 3 - 3]);
}

__weak void pixel_gbrg8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *o0,
					     uint16_t w)
{
	__ASSERT_NO_MSG(w >= 2 && w % 2 == 0);

	for (size_t i = 0, o = 0; i + 3 <= w; i += 2, o += 6) {
		pixel_gbrg8_to_rgb24_2x2(i0[i + 0], i0[i + 1], i1[i + 0], i1[i + 1], &o0[o + 0]);
		pixel_bggr8_to_rgb24_2x2(i0[i + 1], i0[i + 2], i1[i + 1], i1[i + 2], &o0[o + 3]);
	}
	pixel_gbrg8_to_rgb24_2x2(i0[w - 1], i0[w - 2], i1[w - 1], i1[w - 2], &o0[w * 3 - 6]);
	pixel_bggr8_to_rgb24_2x2(i0[w - 2], i0[w - 1], i1[w - 2], i1[w - 1], &o0[w * 3 - 3]);
}

__weak void pixel_bggr8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *o0,
					     uint16_t w)
{
	__ASSERT_NO_MSG(w >= 2 && w % 2 == 0);

	for (size_t i = 0, o = 0; i + 3 <= w; i += 2, o += 6) {
		pixel_bggr8_to_rgb24_2x2(i0[i + 0], i0[i + 1], i1[i + 0], i1[i + 1], &o0[o + 0]);
		pixel_gbrg8_to_rgb24_2x2(i0[i + 1], i0[i + 2], i1[i + 1], i1[i + 2], &o0[o + 3]);
	}
	pixel_bggr8_to_rgb24_2x2(i0[w - 1], i0[w - 2], i1[w - 1], i1[w - 2], &o0[w * 3 - 6]);
	pixel_gbrg8_to_rgb24_2x2(i0[w - 2], i0[w - 1], i1[w - 2], i1[w - 1], &o0[w * 3 - 3]);
}

__weak void pixel_grbg8line_to_rgb24line_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *o0,
					     uint16_t w)
{
	__ASSERT_NO_MSG(w >= 2 && w % 2 == 0);

	for (size_t i = 0, o = 0; i + 3 <= w; i += 2, o += 6) {
		pixel_grbg8_to_rgb24_2x2(i0[i + 0], i0[i + 1], i1[i + 0], i1[i + 1], &o0[o + 0]);
		pixel_rggb8_to_rgb24_2x2(i0[i + 1], i0[i + 2], i1[i + 1], i1[i + 2], &o0[o + 3]);
	}
	pixel_grbg8_to_rgb24_2x2(i0[w - 1], i0[w - 2], i1[w - 1], i1[w - 2], &o0[w * 3 - 6]);
	pixel_rggb8_to_rgb24_2x2(i0[w - 2], i0[w - 1], i1[w - 2], i1[w - 1], &o0[w * 3 - 3]);
}

typedef void fn_3x3_t(const uint8_t *i0, const uint8_t *i1, const uint8_t *i2, uint8_t *o0,
		      uint16_t width);

typedef void fn_2x2_t(const uint8_t *i0, const uint8_t *i1, uint8_t *o0, uint16_t width);

static inline void pixel_bayerstream_to_rgb24stream_3x3(struct pixel_stream *strm, fn_3x3_t *fn0,
							fn_3x3_t *fn1)
{
	uint16_t prev_line_offset = strm->line_offset;
	const uint8_t *i0 = pixel_stream_get_input_line(strm);
	const uint8_t *i1 = pixel_stream_peek_input_line(strm);
	const uint8_t *i2 = pixel_stream_peek_input_line(strm);

	if (prev_line_offset == 0) {
		fn1(i1, i0, i1, pixel_stream_get_output_line(strm), strm->width);
		pixel_stream_done(strm);
	}

	if (prev_line_offset % 2 == 0) {
		fn0(i0, i1, i2, pixel_stream_get_output_line(strm), strm->width);
		pixel_stream_done(strm);
	} else {
		fn1(i0, i1, i2, pixel_stream_get_output_line(strm), strm->width);
		pixel_stream_done(strm);
	}

	if (strm->line_offset + 2 == strm->height) {
		fn0(i1, i2, i1, pixel_stream_get_output_line(strm), strm->width);
		pixel_stream_done(strm);

		/* Skip the two lines of lookahead context, now that the conversion is complete */
		pixel_stream_get_input_line(strm);
		pixel_stream_get_input_line(strm);
	}
}

static inline void pixel_bayerstream_to_rgb24stream_2x2(struct pixel_stream *strm, fn_2x2_t *fn0,
							fn_2x2_t *fn1)
{
	uint16_t prev_line_offset = strm->line_offset;
	const uint8_t *i0 = pixel_stream_get_input_line(strm);
	const uint8_t *i1 = pixel_stream_peek_input_line(strm);

	if (prev_line_offset % 2 == 0) {
		fn0(i0, i1, pixel_stream_get_output_line(strm), strm->width);
		pixel_stream_done(strm);
	} else {
		fn1(i0, i1, pixel_stream_get_output_line(strm), strm->width);
		pixel_stream_done(strm);
	}

	if (strm->line_offset + 1 == strm->height) {
		fn0(i1, i0, pixel_stream_get_output_line(strm), strm->width);
		pixel_stream_done(strm);

		/* Skip the two lines of lookahead context, now that the conversion is complete */
		pixel_stream_get_input_line(strm);
	}
}

void pixel_rggb8stream_to_rgb24stream_3x3(struct pixel_stream *strm)
{
	pixel_bayerstream_to_rgb24stream_3x3(strm, &pixel_rggb8line_to_rgb24line_3x3,
					     &pixel_gbrg8line_to_rgb24line_3x3);
}

void pixel_gbrg8stream_to_rgb24stream_3x3(struct pixel_stream *strm)
{
	pixel_bayerstream_to_rgb24stream_3x3(strm, &pixel_gbrg8line_to_rgb24line_3x3,
					     &pixel_rggb8line_to_rgb24line_3x3);
}

void pixel_bggr8stream_to_rgb24stream_3x3(struct pixel_stream *strm)
{
	pixel_bayerstream_to_rgb24stream_3x3(strm, &pixel_bggr8line_to_rgb24line_3x3,
					     &pixel_grbg8line_to_rgb24line_3x3);
}

void pixel_grbg8stream_to_rgb24stream_3x3(struct pixel_stream *strm)
{
	pixel_bayerstream_to_rgb24stream_3x3(strm, &pixel_grbg8line_to_rgb24line_3x3,
					     &pixel_bggr8line_to_rgb24line_3x3);
}

void pixel_rggb8stream_to_rgb24stream_2x2(struct pixel_stream *strm)
{
	pixel_bayerstream_to_rgb24stream_2x2(strm, &pixel_rggb8line_to_rgb24line_2x2,
					     &pixel_gbrg8line_to_rgb24line_2x2);
}

void pixel_gbrg8stream_to_rgb24stream_2x2(struct pixel_stream *strm)
{
	pixel_bayerstream_to_rgb24stream_2x2(strm, &pixel_gbrg8line_to_rgb24line_2x2,
					     &pixel_rggb8line_to_rgb24line_2x2);
}

void pixel_bggr8stream_to_rgb24stream_2x2(struct pixel_stream *strm)
{
	pixel_bayerstream_to_rgb24stream_2x2(strm, &pixel_bggr8line_to_rgb24line_2x2,
					     &pixel_grbg8line_to_rgb24line_2x2);
}

void pixel_grbg8stream_to_rgb24stream_2x2(struct pixel_stream *strm)
{
	pixel_bayerstream_to_rgb24stream_2x2(strm, &pixel_grbg8line_to_rgb24line_2x2,
					     pixel_bggr8line_to_rgb24line_2x2);
}

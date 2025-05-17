/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/pixel/image.h>
#include <zephyr/pixel/bayer.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pixel_bayer, CONFIG_PIXEL_LOG_LEVEL);

int pixel_image_debayer(struct pixel_image *img, uint32_t window_size)
{
	const struct pixel_operation *op = NULL;

	STRUCT_SECTION_FOREACH_ALTERNATE(pixel_convert, pixel_operation, tmp) {
		if (tmp->fourcc_in == img->fourcc && tmp->fourcc_out == VIDEO_PIX_FMT_RGB24 &&
		    tmp->window_size == window_size) {
			op = tmp;
			break;
		}
	}

	if (op == NULL) {
		LOG_ERR("Conversion operation from %s to %s using %ux%u window not found",
			VIDEO_FOURCC_TO_STR(img->fourcc), VIDEO_FOURCC_TO_STR(VIDEO_PIX_FMT_RGB24),
			window_size, window_size);
		return pixel_image_error(img, -ENOSYS);
	}

	return pixel_image_add_uncompressed(img, op);
}

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

__weak void pixel_line_rggb8_to_rgb24_3x3(const uint8_t *i0, const uint8_t *i1,
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

__weak void pixel_line_grbg8_to_rgb24_3x3(const uint8_t *i0, const uint8_t *i1,
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

__weak void pixel_line_bggr8_to_rgb24_3x3(const uint8_t *i0, const uint8_t *i1,
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

__weak void pixel_line_gbrg8_to_rgb24_3x3(const uint8_t *i0, const uint8_t *i1,
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

__weak void pixel_line_rggb8_to_rgb24_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *o0,
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

__weak void pixel_line_gbrg8_to_rgb24_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *o0,
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

__weak void pixel_line_bggr8_to_rgb24_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *o0,
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

__weak void pixel_line_grbg8_to_rgb24_2x2(const uint8_t *i0, const uint8_t *i1, uint8_t *o0,
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

static inline void pixel_op_bayer_to_rgb24_3x3(struct pixel_operation *op, fn_3x3_t *fn0,
							fn_3x3_t *fn1)
{
	uint16_t prev_line_offset = op->line_offset;
	const uint8_t *i0 = pixel_operation_get_input_line(op);
	const uint8_t *i1 = pixel_operation_peek_input_line(op);
	const uint8_t *i2 = pixel_operation_peek_input_line(op);

	if (prev_line_offset == 0) {
		fn1(i1, i0, i1, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);
	}

	if (prev_line_offset % 2 == 0) {
		fn0(i0, i1, i2, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);
	} else {
		fn1(i0, i1, i2, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);
	}

	if (op->line_offset + 2 == op->height) {
		fn0(i1, i2, i1, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);

		/* Skip the two lines of lookahead context, now that the conversion is complete */
		pixel_operation_get_input_line(op);
		pixel_operation_get_input_line(op);
	}
}

static inline void pixel_op_bayer_to_rgb24_2x2(struct pixel_operation *op, fn_2x2_t *fn0,
							fn_2x2_t *fn1)
{
	uint16_t prev_line_offset = op->line_offset;
	const uint8_t *i0 = pixel_operation_get_input_line(op);
	const uint8_t *i1 = pixel_operation_peek_input_line(op);

	if (prev_line_offset % 2 == 0) {
		fn0(i0, i1, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);
	} else {
		fn1(i0, i1, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);
	}

	if (op->line_offset + 1 == op->height) {
		fn0(i0, i1, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);

		/* Skip the two lines of lookahead context, now that the conversion is complete */
		pixel_operation_get_input_line(op);
	}
}

static void pixel_op_rggb8_to_rgb24_3x3(struct pixel_operation *op)
{
	pixel_op_bayer_to_rgb24_3x3(op, &pixel_line_rggb8_to_rgb24_3x3,
				    &pixel_line_gbrg8_to_rgb24_3x3);
}
PIXEL_DEFINE_BAYER_OPERATION(pixel_op_rggb8_to_rgb24_3x3, VIDEO_PIX_FMT_RGGB8, 3);

static void pixel_op_gbrg8_to_rgb24_3x3(struct pixel_operation *op)
{
	pixel_op_bayer_to_rgb24_3x3(op, &pixel_line_gbrg8_to_rgb24_3x3,
				    &pixel_line_rggb8_to_rgb24_3x3);
}
PIXEL_DEFINE_BAYER_OPERATION(pixel_op_gbrg8_to_rgb24_3x3, VIDEO_PIX_FMT_GBRG8, 3);

static void pixel_op_bggr8_to_rgb24_3x3(struct pixel_operation *op)
{
	pixel_op_bayer_to_rgb24_3x3(op, &pixel_line_bggr8_to_rgb24_3x3,
				    &pixel_line_grbg8_to_rgb24_3x3);
}
PIXEL_DEFINE_BAYER_OPERATION(pixel_op_bggr8_to_rgb24_3x3, VIDEO_PIX_FMT_BGGR8, 3);

static void pixel_op_grbg8_to_rgb24_3x3(struct pixel_operation *op)
{
	pixel_op_bayer_to_rgb24_3x3(op, &pixel_line_grbg8_to_rgb24_3x3,
				    &pixel_line_bggr8_to_rgb24_3x3);
}
PIXEL_DEFINE_BAYER_OPERATION(pixel_op_grbg8_to_rgb24_3x3, VIDEO_PIX_FMT_GRBG8, 3);

static void pixel_op_rggb8_to_rgb24_2x2(struct pixel_operation *op)
{
	pixel_op_bayer_to_rgb24_2x2(op, &pixel_line_rggb8_to_rgb24_2x2,
				    &pixel_line_gbrg8_to_rgb24_2x2);
}
PIXEL_DEFINE_BAYER_OPERATION(pixel_op_rggb8_to_rgb24_2x2, VIDEO_PIX_FMT_RGGB8, 2);

static void pixel_op_gbrg8_to_rgb24_2x2(struct pixel_operation *op)
{
	pixel_op_bayer_to_rgb24_2x2(op, &pixel_line_gbrg8_to_rgb24_2x2,
				    &pixel_line_rggb8_to_rgb24_2x2);
}
PIXEL_DEFINE_BAYER_OPERATION(pixel_op_gbrg8_to_rgb24_2x2, VIDEO_PIX_FMT_GBRG8, 2);

static void pixel_op_bggr8_to_rgb24_2x2(struct pixel_operation *op)
{
	pixel_op_bayer_to_rgb24_2x2(op, &pixel_line_bggr8_to_rgb24_2x2,
				    &pixel_line_grbg8_to_rgb24_2x2);
}
PIXEL_DEFINE_BAYER_OPERATION(pixel_op_bggr8_to_rgb24_2x2, VIDEO_PIX_FMT_BGGR8, 2);

static void pixel_op_grbg8_to_rgb24_2x2(struct pixel_operation *op)
{
	pixel_op_bayer_to_rgb24_2x2(op, &pixel_line_grbg8_to_rgb24_2x2,
				    pixel_line_bggr8_to_rgb24_2x2);
}
PIXEL_DEFINE_BAYER_OPERATION(pixel_op_grbg8_to_rgb24_2x2, VIDEO_PIX_FMT_GRBG8, 2);

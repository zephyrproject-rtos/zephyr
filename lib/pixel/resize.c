/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include <zephyr/sys/util.h>
#include <zephyr/pixel/resize.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pixel_resize, CONFIG_PIXEL_LOG_LEVEL);

static inline void pixel_subsample_line(const uint8_t *src_buf, size_t src_width,
					       uint8_t *dst_buf, size_t dst_width,
					       uint8_t bits_per_pixel)
{
	for (size_t dst_w = 0; dst_w < dst_width; dst_w++) {
		size_t src_w = dst_w * src_width / dst_width;
		size_t src_i = src_w * bits_per_pixel / BITS_PER_BYTE;
		size_t dst_i = dst_w * bits_per_pixel / BITS_PER_BYTE;

		memmove(&dst_buf[dst_i], &src_buf[src_i], bits_per_pixel / BITS_PER_BYTE);
	}
}

static inline void pixel_subsample_frame(const uint8_t *src_buf, size_t src_width,
					 size_t src_height, uint8_t *dst_buf, size_t dst_width,
					 size_t dst_height, uint8_t bits_per_pixel)
{
	for (size_t dst_h = 0; dst_h < dst_height; dst_h++) {
		size_t src_h = dst_h * src_height / dst_height;
		size_t src_i = src_h * src_width * bits_per_pixel / BITS_PER_BYTE;
		size_t dst_i = dst_h * dst_width * bits_per_pixel / BITS_PER_BYTE;

		pixel_subsample_line(&src_buf[src_i], src_width, &dst_buf[dst_i], dst_width,
				     bits_per_pixel);
	}
}

__weak void pixel_subsample_rgb24frame(const uint8_t *src_buf, size_t src_width, size_t src_height,
				       uint8_t *dst_buf, size_t dst_width, size_t dst_height)
{
	pixel_subsample_frame(src_buf, src_width, src_height, dst_buf, dst_width, dst_height, 24);
}

__weak void pixel_subsample_rgb565frame(const uint8_t *src_buf, size_t src_width, size_t src_height,
					uint8_t *dst_buf, size_t dst_width, size_t dst_height)
{
	pixel_subsample_frame(src_buf, src_width, src_height, dst_buf, dst_width, dst_height, 16);
}

static inline void pixel_subsample_stream(struct pixel_stream *strm, uint8_t bits_per_pixel)
{
	uint16_t prev_offset = (strm->line_offset + 1) * strm->next->height / strm->height;
	const uint8_t *line_in = pixel_stream_get_input_line(strm);
	uint16_t next_offset = (strm->line_offset + 1) * strm->next->height / strm->height;

	for (uint16_t i = 0; prev_offset + i < next_offset; i++) {
		pixel_subsample_line(line_in, strm->width, pixel_stream_get_output_line(strm),
				     strm->next->width, bits_per_pixel);
		pixel_stream_done(strm);
	}
}

__weak void pixel_subsample_rgb24stream(struct pixel_stream *strm)
{
	pixel_subsample_stream(strm, 24);
}

__weak void pixel_subsample_rgb565stream(struct pixel_stream *strm)
{
	pixel_subsample_stream(strm, 16);
}

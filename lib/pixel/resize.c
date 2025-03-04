/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/pixel/resize.h>
#include <zephyr/pixel/image.h>

LOG_MODULE_REGISTER(pixel_resize, CONFIG_PIXEL_LOG_LEVEL);

int pixel_image_resize(struct pixel_image *img, uint16_t width, uint16_t height)
{
	const struct pixel_operation *op = NULL;
	int ret;

	STRUCT_SECTION_FOREACH_ALTERNATE(pixel_resize, pixel_operation, tmp) {
		if (tmp->fourcc_in == img->fourcc) {
			op = tmp;
			break;
		}
	}

	if (op == NULL) {
		LOG_ERR("Resize operation for %s not found", VIDEO_FOURCC_TO_STR(img->fourcc));
		return pixel_image_error(img, -ENOSYS);
	}

	ret = pixel_image_add_uncompressed(img, op);
	img->width = width;
	img->height = height;
	return ret;
}

static inline void pixel_resize_line(const uint8_t *src_buf, size_t src_width, uint8_t *dst_buf,
				     size_t dst_width, uint8_t bits_per_pixel)
{
	for (size_t dst_w = 0; dst_w < dst_width; dst_w++) {
		size_t src_w = dst_w * src_width / dst_width;
		size_t src_i = src_w * bits_per_pixel / BITS_PER_BYTE;
		size_t dst_i = dst_w * bits_per_pixel / BITS_PER_BYTE;

		memmove(&dst_buf[dst_i], &src_buf[src_i], bits_per_pixel / BITS_PER_BYTE);
	}
}

static inline void pixel_resize_frame(const uint8_t *src_buf, size_t src_width, size_t src_height,
				      uint8_t *dst_buf, size_t dst_width, size_t dst_height,
				      uint8_t bits_per_pixel)
{
	for (size_t dst_h = 0; dst_h < dst_height; dst_h++) {
		size_t src_h = dst_h * src_height / dst_height;
		size_t src_i = src_h * src_width * bits_per_pixel / BITS_PER_BYTE;
		size_t dst_i = dst_h * dst_width * bits_per_pixel / BITS_PER_BYTE;

		pixel_resize_line(&src_buf[src_i], src_width, &dst_buf[dst_i], dst_width,
				     bits_per_pixel);
	}
}

__weak void pixel_resize_frame_raw24(const uint8_t *src_buf, size_t src_width, size_t src_height,
				    uint8_t *dst_buf, size_t dst_width, size_t dst_height)
{
	pixel_resize_frame(src_buf, src_width, src_height, dst_buf, dst_width, dst_height, 24);
}

__weak void pixel_resize_frame_raw16(const uint8_t *src_buf, size_t src_width, size_t src_height,
				     uint8_t *dst_buf, size_t dst_width, size_t dst_height)
{
	pixel_resize_frame(src_buf, src_width, src_height, dst_buf, dst_width, dst_height, 16);
}

__weak void pixel_resize_frame_raw8(const uint8_t *src_buf, size_t src_width, size_t src_height,
				    uint8_t *dst_buf, size_t dst_width, size_t dst_height)
{
	pixel_resize_frame(src_buf, src_width, src_height, dst_buf, dst_width, dst_height, 8);
}

static inline void pixel_op_resize(struct pixel_operation *op, uint8_t bits_per_pixel)
{
	struct pixel_operation *next = SYS_SLIST_PEEK_NEXT_CONTAINER(op, node);
	uint16_t prev_offset = (op->line_offset + 1) * next->height / op->height;
	const uint8_t *line_in = pixel_operation_get_input_line(op);
	uint16_t next_offset = (op->line_offset + 1) * next->height / op->height;

	for (uint16_t i = 0; prev_offset + i < next_offset; i++) {
		pixel_resize_line(line_in, op->width, pixel_operation_get_output_line(op),
				     next->width, bits_per_pixel);
		pixel_operation_done(op);
	}
}

__weak void pixel_op_resize_raw32(struct pixel_operation *op)
{
	pixel_op_resize(op, 32);
}
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw32, VIDEO_PIX_FMT_ABGR32);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw32, VIDEO_PIX_FMT_ARGB32);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw32, VIDEO_PIX_FMT_BGRA32);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw32, VIDEO_PIX_FMT_RGBA32);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw32, VIDEO_PIX_FMT_XRGB32);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw32, VIDEO_PIX_FMT_XYUV32);

__weak void pixel_op_resize_raw24(struct pixel_operation *op)
{
	pixel_op_resize(op, 24);
}
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw24, VIDEO_PIX_FMT_BGR24);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw24, VIDEO_PIX_FMT_RGB24);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw24, VIDEO_PIX_FMT_YUV24);

__weak void pixel_op_resize_raw16(struct pixel_operation *op)
{
	pixel_op_resize(op, 16);
}
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw16, VIDEO_PIX_FMT_RGB565);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw16, VIDEO_PIX_FMT_RGB565X);

__weak void pixel_op_resize_raw8(struct pixel_operation *op)
{
	pixel_op_resize(op, 8);
}
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw8, VIDEO_PIX_FMT_GREY);
PIXEL_DEFINE_RESIZE_OPERATION(pixel_op_resize_raw8, VIDEO_PIX_FMT_RGB332);

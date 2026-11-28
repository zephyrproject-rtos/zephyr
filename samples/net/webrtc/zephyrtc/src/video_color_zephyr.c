/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "video_color_zephyr.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include "video_codec_zephyr.h"

static uint8_t clip_u8(int32_t value)
{
	if (value < 0) {
		return 0U;
	}
	if (value > 255) {
		return 255U;
	}
	return (uint8_t)value;
}

void video_color_bgrx32_to_i420(const uint8_t *bgrx, uint8_t *i420)
{
	uint8_t *y = i420;
	uint8_t *u = y + VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT;
	uint8_t *v = u + VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT / 4U;

	for (uint32_t row = 0U; row < VIDEO_CODEC_HEIGHT; row += 2U) {
		for (uint32_t column = 0U; column < VIDEO_CODEC_WIDTH; column += 2U) {
			int32_t red_sum = 0;
			int32_t green_sum = 0;
			int32_t blue_sum = 0;

			for (uint32_t y_offset = 0U; y_offset < 2U; y_offset++) {
				for (uint32_t x_offset = 0U; x_offset < 2U; x_offset++) {
					uint32_t pixel_index =
						(row + y_offset) * VIDEO_CODEC_WIDTH + column +
						x_offset;
					const uint8_t *pixel = bgrx + pixel_index * 4U;
					int32_t blue = pixel[0];
					int32_t green = pixel[1];
					int32_t red = pixel[2];

					y[pixel_index] = clip_u8(
						((66 * red + 129 * green + 25 * blue + 128) >> 8) +
						16);
					red_sum += red;
					green_sum += green;
					blue_sum += blue;
				}
			}
			red_sum /= 4;
			green_sum /= 4;
			blue_sum /= 4;
			uint32_t chroma_index = (row / 2U) * (VIDEO_CODEC_WIDTH / 2U) + column / 2U;

			u[chroma_index] = clip_u8(
				((-38 * red_sum - 74 * green_sum + 112 * blue_sum + 128) >> 8) +
				128);
			v[chroma_index] = clip_u8(
				((112 * red_sum - 94 * green_sum - 18 * blue_sum + 128) >> 8) +
				128);
		}
	}
}

int video_color_rgb565_dimensions(uint32_t rotation, uint32_t scale, uint32_t *width,
				  uint32_t *height)
{
	if (width == NULL || height == NULL || scale == 0U || scale > VIDEO_COLOR_MAX_SCALE) {
		return -EINVAL;
	}
	if (rotation == 0U || rotation == 180U) {
		*width = VIDEO_CODEC_WIDTH * scale;
		*height = VIDEO_CODEC_HEIGHT * scale;
	} else if (rotation == 90U || rotation == 270U) {
		*width = VIDEO_CODEC_HEIGHT * scale;
		*height = VIDEO_CODEC_WIDTH * scale;
	} else {
		return -EINVAL;
	}
	return 0;
}

static uint16_t i420_pixel_to_rgb565(const uint8_t *i420, uint32_t column, uint32_t row)
{
	const uint8_t *y = i420;
	const uint8_t *u = y + VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT;
	const uint8_t *v = u + VIDEO_CODEC_WIDTH * VIDEO_CODEC_HEIGHT / 4U;
	uint32_t luma_index = row * VIDEO_CODEC_WIDTH + column;
	uint32_t chroma_index = (row / 2U) * (VIDEO_CODEC_WIDTH / 2U) + column / 2U;
	int32_t luma = (int32_t)y[luma_index] - 16;
	int32_t chroma_u = (int32_t)u[chroma_index] - 128;
	int32_t chroma_v = (int32_t)v[chroma_index] - 128;

	if (luma < 0) {
		luma = 0;
	}
	uint8_t red = clip_u8((298 * luma + 409 * chroma_v + 128) >> 8);
	uint8_t green = clip_u8((298 * luma - 100 * chroma_u - 208 * chroma_v + 128) >> 8);
	uint8_t blue = clip_u8((298 * luma + 516 * chroma_u + 128) >> 8);

	return (uint16_t)(((uint16_t)(red & 0xf8U) << 8) | ((uint16_t)(green & 0xfcU) << 3) |
			  ((uint16_t)blue >> 3));
}

int video_color_i420_to_rgb565(const uint8_t *i420, uint16_t *rgb565, uint32_t rotation,
			       uint32_t scale)
{
	uint32_t output_width;
	uint32_t output_height;
	uint32_t rotated_width;
	uint32_t rotated_height;

	if (i420 == NULL || rgb565 == NULL ||
	    video_color_rgb565_dimensions(rotation, scale, &output_width, &output_height) != 0) {
		return -EINVAL;
	}
	rotated_width = output_width / scale;
	rotated_height = output_height / scale;
	for (uint32_t row = 0U; row < rotated_height; row++) {
		for (uint32_t column = 0U; column < rotated_width; column++) {
			uint32_t source_column;
			uint32_t source_row;

			switch (rotation) {
			case 0U:
				source_column = column;
				source_row = row;
				break;
			case 90U:
				source_column = row;
				source_row = VIDEO_CODEC_HEIGHT - 1U - column;
				break;
			case 180U:
				source_column = VIDEO_CODEC_WIDTH - 1U - column;
				source_row = VIDEO_CODEC_HEIGHT - 1U - row;
				break;
			default:
				source_column = VIDEO_CODEC_WIDTH - 1U - row;
				source_row = column;
				break;
			}
			uint16_t pixel = i420_pixel_to_rgb565(i420, source_column, source_row);
			uint32_t output_column = column * scale;
			uint32_t output_row = row * scale;

			for (uint32_t y = 0U; y < scale; y++) {
				uint16_t *destination =
					rgb565 + (output_row + y) * output_width + output_column;

				for (uint32_t x = 0U; x < scale; x++) {
					destination[x] = pixel;
				}
			}
		}
	}
	return 0;
}

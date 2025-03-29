/*
 * Copyir (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/pixel/formats.h>

LOG_MODULE_REGISTER(pixel_formats, CONFIG_PIXEL_LOG_LEVEL);

void pixel_rgb565leline_to_rgb24line(const uint8_t *rgb565, uint8_t *rgb24, uint16_t width)
{
	for (uint16_t w = 0; w < width; w++) {
		pixel_rgb565le_to_rgb24(&rgb565[w * 2], &rgb24[w * 3]);
	}
}

void pixel_rgb565beline_to_rgb24line(const uint8_t *rgb565, uint8_t *rgb24, uint16_t width)
{
	for (uint16_t w = 0; w < width; w++) {
		pixel_rgb565be_to_rgb24(&rgb565[w * 2], &rgb24[w * 3]);
	}
}

void pixel_rgb24line_to_rgb565leline(const uint8_t *rgb24, uint8_t *rgb565, uint16_t width)
{
	for (uint16_t w = 0; w < width; w++) {
		pixel_rgb24_to_rgb565le(&rgb24[w * 3], &rgb565[w * 2]);
	}
}

void pixel_rgb24line_to_rgb565beline(const uint8_t *rgb24, uint8_t *rgb565, uint16_t width)
{
	for (uint16_t w = 0; w < width; w++) {
		pixel_rgb24_to_rgb565be(&rgb24[w * 3], &rgb565[w * 2]);
	}
}

void pixel_rgb24line_to_yuyvline_bt601(const uint8_t *rgb24, uint8_t *yuyv, uint16_t width)
{
	for (uint16_t w = 0; w + 2 <= width; w += 2) {
		pixel_rgb24x2_to_yuyv_bt601(&rgb24[w * 3], &yuyv[w * 2]);
	}
}

void pixel_rgb24line_to_yuyvline_bt709(const uint8_t *rgb24, uint8_t *yuyv, uint16_t width)
{
	for (uint16_t w = 0; w + 2 <= width; w += 2) {
		pixel_rgb24x2_to_yuyv_bt709(&rgb24[w * 3], &yuyv[w * 2]);
	}
}

void pixel_yuyvline_to_rgb24line_bt601(const uint8_t *yuyv, uint8_t *rgb24, uint16_t width)
{
	for (uint16_t w = 0; w + 2 <= width; w += 2) {
		pixel_yuyv_to_rgb24x2_bt601(&yuyv[w * 2], &rgb24[w * 3]);
	}
}

void pixel_yuyvline_to_rgb24line_bt709(const uint8_t *yuyv, uint8_t *rgb24, uint16_t width)
{
	for (uint16_t w = 0; w + 2 <= width; w += 2) {
		pixel_yuyv_to_rgb24x2_bt709(&yuyv[w * 2], &rgb24[w * 3]);
	}
}

void pixel_rgb24stream_to_rgb565lestream(struct pixel_stream *strm)
{
	pixel_rgb24line_to_rgb565leline(pixel_stream_get_input_line(strm),
					pixel_stream_get_output_line(strm), strm->pitch / 3);
	pixel_stream_done(strm);
}

void pixel_rgb24stream_to_rgb565bestream(struct pixel_stream *strm)
{
	pixel_rgb24line_to_rgb565beline(pixel_stream_get_input_line(strm),
					pixel_stream_get_output_line(strm), strm->pitch / 3);
	pixel_stream_done(strm);
}

void pixel_rgb565lestream_to_rgb24stream(struct pixel_stream *strm)
{
	pixel_rgb565leline_to_rgb24line(pixel_stream_get_input_line(strm),
					pixel_stream_get_output_line(strm), strm->pitch / 2);
	pixel_stream_done(strm);
}

void pixel_rgb565bestream_to_rgb24stream(struct pixel_stream *strm)
{
	pixel_rgb565beline_to_rgb24line(pixel_stream_get_input_line(strm),
					pixel_stream_get_output_line(strm), strm->pitch / 2);
	pixel_stream_done(strm);
}

void pixel_yuyvstream_to_rgb24stream_bt601(struct pixel_stream *strm)
{
	pixel_yuyvline_to_rgb24line_bt601(pixel_stream_get_input_line(strm),
					  pixel_stream_get_output_line(strm), strm->pitch / 2);
	pixel_stream_done(strm);
}

void pixel_yuyvstream_to_rgb24stream_bt709(struct pixel_stream *strm)
{
	pixel_yuyvline_to_rgb24line_bt709(pixel_stream_get_input_line(strm),
					  pixel_stream_get_output_line(strm), strm->pitch / 2);
	pixel_stream_done(strm);
}

void pixel_rgb24stream_to_yuyvstream_bt601(struct pixel_stream *strm)
{
	pixel_rgb24line_to_yuyvline_bt601(pixel_stream_get_input_line(strm),
					  pixel_stream_get_output_line(strm), strm->pitch / 3);
	pixel_stream_done(strm);
}

void pixel_rgb24stream_to_yuyvstream_bt709(struct pixel_stream *strm)
{
	pixel_rgb24line_to_yuyvline_bt709(pixel_stream_get_input_line(strm),
					  pixel_stream_get_output_line(strm), strm->pitch / 3);
	pixel_stream_done(strm);
}

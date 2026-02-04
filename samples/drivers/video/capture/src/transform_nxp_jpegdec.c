/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include "transform.h"

#ifdef CONFIG_VIDEO_NXP_JPEGDEC_CSC
static uint8_t BYTECLIP(int val)
{
	if (val < 0) {
		return 0U;
	} else if (val > 255) {
		return 255U;
	} else {
		return (uint8_t)val;
	}
}

static void Convert_yuv420_to_rgb565(uint16_t width, uint16_t height, uint32_t yAddr,
				     uint32_t uvAddr, uint32_t rgbAddr)
{
	uint16_t *rgb = (void *)rgbAddr;
	int16_t r, g, b;
	uint8_t R, G, B, y, u, v;

	uint16_t i, j;

	for (i = 0U; i < height; i++) {
		for (j = 0U; j < width; j++) {
			y = *(((uint8_t *)yAddr) + width * i + j);
			u = *(((uint8_t *)uvAddr) + width * (i / 2) + j - (j % 2));
			v = *(((uint8_t *)uvAddr) + width * (i / 2) + j + 1 - (j % 2));
			r = y + 1402 * (v - 128) / 1000;
			g = y - (344 * (u - 128) + 714 * (v - 128)) / 1000;
			b = y + 1772 * (u - 128) / 1000;
			R = BYTECLIP(r);
			G = BYTECLIP(g);
			B = BYTECLIP(b);
			*rgb++ = (uint16_t)((((uint16_t)R & 0xF8) << 8U) |
					    (((uint16_t)G & 0xFC) << 3U) |
					    (((uint16_t)B & 0xF8) >> 3U));
		}
	}
}
#endif

int app_setup_video_transform(const struct device *const transform_dev, struct video_format in_fmt,
			      struct video_format *const out_fmt, struct video_buffer **out_buf)
{
	out_fmt->pixelformat = VIDEO_PIX_FMT_NV12; /* Decoder output format shall be NV12 */
	out_fmt->width = CONFIG_VIDEO_FRAME_WIDTH;
	out_fmt->height = CONFIG_VIDEO_FRAME_HEIGHT;
	out_fmt->pitch = out_fmt->width;
	out_fmt->size = out_fmt->width * out_fmt->height * 2U;

	int ret = setup_video_transform(transform_dev, in_fmt, out_fmt, out_buf);

#ifdef CONFIG_VIDEO_NXP_JPEGDEC_CSC
	if (ret < 0) {
		return ret;
	}

	/* The transfromed format shall be RGB565 for display panel to show. */
	out_fmt->pixelformat = VIDEO_PIX_FMT_RGB565;
	out_fmt->pitch = out_fmt->width * 2U;

	return 0;
#else
	return ret;
#endif
}

int app_transform_frame(const struct device *const transform_dev, struct video_buffer *in_buf,
			struct video_buffer **out_buf)
{
	int ret = transform_frame(transform_dev, in_buf, out_buf);

#ifdef CONFIG_VIDEO_NXP_JPEGDEC_CSC
	if (ret < 0) {
		return ret;
	}

	/* Change the decoded NV12 2-p pixel to RGB565 for display to show. */
	uint8_t *rgb565 = k_aligned_alloc(CONFIG_VIDEO_BUFFER_POOL_ALIGN, (*out_buf)->bytesused);

	Convert_yuv420_to_rgb565(CONFIG_VIDEO_FRAME_WIDTH, CONFIG_VIDEO_FRAME_HEIGHT,
				 (uint32_t)(*out_buf)->buffer,
				 (uint32_t)((*out_buf)->buffer +
					    CONFIG_VIDEO_FRAME_WIDTH * CONFIG_VIDEO_FRAME_HEIGHT),
				 (uint32_t)rgb565);

	memcpy((*out_buf)->buffer, rgb565,
	       CONFIG_VIDEO_FRAME_WIDTH * CONFIG_VIDEO_FRAME_HEIGHT * 2U);

	return 0;
#else
	return ret;
#endif
}

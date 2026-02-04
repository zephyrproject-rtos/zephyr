/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include "transform.h"


int app_setup_video_transform(const struct device *const transform_dev, struct video_format in_fmt,
			      struct video_format *const out_fmt, struct video_buffer **out_buf)
{
	out_fmt->pixelformat = VIDEO_PIX_FMT_ABGR32; /* Decoder output format shall be ABGR8888 */
	out_fmt->width = CONFIG_VIDEO_FRAME_WIDTH;
	out_fmt->height = CONFIG_VIDEO_FRAME_HEIGHT;
	out_fmt->pitch = out_fmt->width * 4;
	out_fmt->size = out_fmt->pitch * out_fmt->height;

	return setup_video_transform(transform_dev, in_fmt, out_fmt, out_buf);
}

int app_transform_frame(const struct device *const transform_dev, struct video_buffer *in_buf,
			struct video_buffer **out_buf)
{
	return transform_frame(transform_dev, in_buf, out_buf);
}

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(transform_nxp_jpegdec, CONFIG_LOG_DEFAULT_LEVEL);

#include "transform.h"

int app_setup_video_transform(const struct device *const transform_dev,
	struct video_format *const in_fmt,
	struct video_format *const out_fmt,
	struct video_buffer **out_buf)
{
	out_fmt->pixelformat = VIDEO_PIX_FMT_NV12; /* Decoder output format shall be NV12 */
	out_fmt->width = CONFIG_VIDEO_FRAME_WIDTH;
	out_fmt->height = CONFIG_VIDEO_FRAME_HEIGHT;
	out_fmt->pitch = out_fmt->width;
	out_fmt->size = out_fmt->width * out_fmt->height * 2U;

	int ret = setup_video_transform(transform_dev, in_fmt, out_fmt, out_buf);

	return ret;
}

int app_transform_frame(const struct device *const transform_dev, struct video_buffer *in_buf,
			struct video_buffer **out_buf)
{
	int ret = transform_frame(transform_dev, in_buf, out_buf);

	return ret;
}

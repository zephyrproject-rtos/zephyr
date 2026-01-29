/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>

#include "transform.h"

int app_setup_video_transform(const struct device *const transform_dev, struct video_format in_fmt,
			      struct video_format *const out_fmt, struct video_buffer **out_buf)
{
	struct video_control ctrl = {.id = VIDEO_CID_ROTATE, .val = 90};

	/* Set controls */
	video_set_ctrl(transform_dev, &ctrl);

	*out_fmt = in_fmt;
	out_fmt->width = in_fmt.height;
	out_fmt->height = in_fmt.width;
	out_fmt->type = VIDEO_BUF_TYPE_OUTPUT;

	return setup_video_transform(transform_dev, in_fmt, out_fmt, out_buf);
}

int app_transform_frame(const struct device *const transform_dev, struct video_buffer *in_buf,
			struct video_buffer **out_buf)
{
	return transform_frame(transform_dev, in_buf, out_buf);
}

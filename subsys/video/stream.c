/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/video/video.h>
#include <zephyr/drivers/video.h>

int video_stream_start(const struct device *dev, enum video_buf_type type)
{
	if (dev == NULL || (type != VIDEO_BUF_TYPE_INPUT && type != VIDEO_BUF_TYPE_OUTPUT)) {
		return -EINVAL;
	}

	return video_driver_set_stream(dev, true, type);
}

int video_stream_stop(const struct device *dev, enum video_buf_type type)
{
	int ret;

	if (dev == NULL || (type != VIDEO_BUF_TYPE_INPUT && type != VIDEO_BUF_TYPE_OUTPUT)) {
		return -EINVAL;
	}

	ret = video_driver_set_stream(dev, false, type);

	video_driver_flush(dev, true);

	return ret;
}

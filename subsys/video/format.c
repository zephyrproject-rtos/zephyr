/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_format, CONFIG_VIDEO_LOG_LEVEL);

int video_set_selection(const struct device *dev, struct video_selection *sel)
{
	if (dev == NULL || sel == NULL) {
		return -EINVAL;
	}

	return video_driver_set_selection(dev, sel);
}

int video_get_selection(const struct device *dev, struct video_selection *sel)
{
	if (dev == NULL || sel == NULL) {
		return -EINVAL;
	}

	return video_driver_get_selection(dev, sel);
}

int video_set_format(const struct device *dev, struct video_format *fmt)
{
	if (dev == NULL || fmt == NULL) {
		return -EINVAL;
	}

	return video_driver_set_format(dev, fmt);
}

int video_get_format(const struct device *dev, struct video_format *fmt)
{
	if (dev == NULL || fmt == NULL) {
		return -EINVAL;
	}

	return video_driver_get_format(dev, fmt);
}

/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_frmival, CONFIG_VIDEO_LOG_LEVEL);

int video_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	if (dev == NULL || frmival == NULL) {
		return -EINVAL;
	}

	return video_driver_set_frmival(dev, frmival);
}

int video_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	if (dev == NULL || frmival == NULL) {
		return -EINVAL;
	}

	return video_driver_get_frmival(dev, frmival);
}

int video_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	if (dev == NULL || fie == NULL) {
		return -EINVAL;
	}

	return video_driver_enum_frmival(dev, fie);
}

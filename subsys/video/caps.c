/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_caps, CONFIG_VIDEO_LOG_LEVEL);

int video_get_caps(const struct device *dev, struct video_caps *caps)
{
	return video_driver_get_caps(dev, caps);
}

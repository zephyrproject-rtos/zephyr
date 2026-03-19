/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_buffer, CONFIG_VIDEO_LOG_LEVEL);

int video_dequeue(const struct device *dev, struct video_buffer **vbuf,
				       k_timeout_t timeout)
{
	if (dev == NULL || vbuf == NULL) {
		return -EINVAL;
	}

	return video_driver_dequeue(dev, vbuf, timeout);
}

int video_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	if (dev == NULL || sig == NULL) {
		return -EINVAL;
	}

	return video_driver_set_signal(dev, sig);
}

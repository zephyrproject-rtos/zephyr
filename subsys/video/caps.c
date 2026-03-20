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

int video_format_caps_index(const struct video_format_cap *fmts, const struct video_format *fmt,
			    size_t *idx)
{
	__ASSERT_NO_MSG(fmts != NULL);
	__ASSERT_NO_MSG(fmt != NULL);
	__ASSERT_NO_MSG(idx != NULL);

	for (int i = 0; fmts[i].pixelformat != 0; i++) {
		if (fmts[i].pixelformat == fmt->pixelformat &&
		    IN_RANGE(fmt->width, fmts[i].width_min, fmts[i].width_max) &&
		    IN_RANGE(fmt->height, fmts[i].height_min, fmts[i].height_max)) {
			*idx = i;
			return 0;
		}
	}
	return -ENOENT;
}

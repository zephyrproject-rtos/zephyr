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

int video_closest_frmival_stepwise(const struct video_frmival_stepwise *stepwise, uint32_t desired,
				   uint32_t *match)
{
	uint32_t goal;

	if (stepwise == NULL || match == NULL) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(stepwise->step != 0U);
	/* Prevent division by zero */
	if (stepwise->step == 0U) {
		return -ERANGE;
	}

	/* Saturate the desired value to the min/max supported */
	goal = CLAMP(desired, stepwise->min, stepwise->max);

	/* Compute the closest match */
	*match = stepwise->min +
		 DIV_ROUND_CLOSEST(goal - stepwise->min, stepwise->step) * stepwise->step;

	return 0;
}

int video_closest_frmival(const struct device *dev, struct video_frmival_enum *match)
{
	if (dev == NULL || match == NULL || match->type == VIDEO_FRMIVAL_TYPE_STEPWISE) {
		return -EINVAL;
	}

	struct video_frmival desired = match->discrete;
	struct video_frmival_enum fie = {.format = match->format};
	uint32_t best_diff_usec = UINT32_MAX;
	uint32_t goal_usec = desired.usec;

	for (fie.index = 0; video_enum_frmival(dev, &fie) == 0; fie.index++) {
		struct video_frmival tmp = {0};
		uint32_t diff_usec = 0;
		uint32_t tmp_usec = 0;
		int ret;

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			tmp = fie.discrete;
			tmp_usec = tmp.usec;
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			ret = video_closest_frmival_stepwise(&fie.stepwise, goal_usec, &tmp_usec);
			if (ret != 0) {
				continue;
			}
			tmp.usec = tmp_usec;
			break;
		default:
			CODE_UNREACHABLE;
		}

		diff_usec = tmp_usec > goal_usec ? tmp_usec - goal_usec : goal_usec - tmp_usec;

		if (diff_usec < best_diff_usec) {
			best_diff_usec = diff_usec;
			match->index = fie.index;
			match->discrete = tmp;
		}

		if (diff_usec == 0) {
			/* Exact match, stop searching a better match */
			break;
		}
	}

	return 0;
}

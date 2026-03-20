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

void video_closest_frmival_stepwise(const struct video_frmival_stepwise *stepwise,
				    const struct video_frmival *desired,
				    struct video_frmival *match)
{
	__ASSERT_NO_MSG(stepwise != NULL);
	__ASSERT_NO_MSG(desired != NULL);
	__ASSERT_NO_MSG(match != NULL);

	uint64_t min = stepwise->min.numerator;
	uint64_t max = stepwise->max.numerator;
	uint64_t step = stepwise->step.numerator;
	uint64_t goal = desired->numerator;

	/* Set a common denominator to all values */
	min *= stepwise->max.denominator * stepwise->step.denominator * desired->denominator;
	max *= stepwise->min.denominator * stepwise->step.denominator * desired->denominator;
	step *= stepwise->min.denominator * stepwise->max.denominator * desired->denominator;
	goal *= stepwise->min.denominator * stepwise->max.denominator * stepwise->step.denominator;

	__ASSERT_NO_MSG(step != 0U);
	/* Prevent division by zero */
	if (step == 0U) {
		return;
	}
	/* Saturate the desired value to the min/max supported */
	goal = CLAMP(goal, min, max);

	/* Compute a numerator and denominator */
	match->numerator = min + DIV_ROUND_CLOSEST(goal - min, step) * step;
	match->denominator = stepwise->min.denominator * stepwise->max.denominator *
			     stepwise->step.denominator * desired->denominator;
}

void video_closest_frmival(const struct device *dev, struct video_frmival_enum *match)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(match != NULL);

	struct video_frmival desired = match->discrete;
	struct video_frmival_enum fie = {.format = match->format};
	uint64_t best_diff_nsec = INT32_MAX;
	uint64_t goal_nsec = video_frmival_nsec(&desired);

	__ASSERT(match->type != VIDEO_FRMIVAL_TYPE_STEPWISE,
		 "cannot find range matching the range, only a value matching the range");

	for (fie.index = 0; video_enum_frmival(dev, &fie) == 0; fie.index++) {
		struct video_frmival tmp = {0};
		uint64_t diff_nsec = 0;
		uint64_t tmp_nsec;

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			tmp = fie.discrete;
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			video_closest_frmival_stepwise(&fie.stepwise, &desired, &tmp);
			break;
		default:
			CODE_UNREACHABLE;
		}

		tmp_nsec = video_frmival_nsec(&tmp);
		diff_nsec = tmp_nsec > goal_nsec ? tmp_nsec - goal_nsec : goal_nsec - tmp_nsec;

		if (diff_nsec < best_diff_nsec) {
			best_diff_nsec = diff_nsec;
			match->index = fie.index;
			match->discrete = tmp;
		}

		if (diff_nsec == 0) {
			/* Exact match, stop searching a better match */
			break;
		}
	}
}

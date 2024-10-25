/*
 * Copyright (c) 2024, tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_COMMON_H
#define ZEPHYR_INCLUDE_VIDEO_COMMON_H

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/drivers/video.h>

/**
 * @brief Search for a format that matches in a list of capabilities
 *
 * @param fmts The format capability list to search.
 * @param fmts_num The number of capabilities in that list.
 * @param fmt The format to find in the list.
 * @param idx The pointer to a number of the first format that matches.
 *
 * @return 0 when a format is found.
 * @return -ENOENT when no matching format is found.
 */
static inline int video_format_caps_index(const struct video_format_cap *fmts,
					  const struct video_format *fmt, size_t *idx)
{
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

/**
 * @brief Compute the difference between two frame intervals
 *
 * @param frmival Frame interval to turn into microseconds.
 *
 * @return The frame interval value in microseconds.
 */
static inline uint64_t video_frmival_nsec(const struct video_frmival *frmival)
{
	return (uint64_t)NSEC_PER_SEC * frmival->numerator / frmival->denominator;
}

/**
 * @brief Find the closest match to a frame interval value within a stepwise frame interval.
 *
 * @param stepwise The stepwise frame interval range to search
 * @param desired The frame interval for which find the closest match
 * @param best_match The resulting frame interval closest to @p desired
 */
static inline void video_closest_frmival_stepwise(const struct video_frmival_stepwise *stepwise,
						  const struct video_frmival *desired,
						  struct video_frmival *match)
{
	uint64_t min = stepwise->min.numerator;
	uint64_t max = stepwise->max.numerator;
	uint64_t step = stepwise->step.numerator;
	uint64_t goal = desired->numerator;

	/* Set a common denominator to all values */
	min *= stepwise->max.denominator * stepwise->step.denominator * desired->denominator;
	max *= stepwise->min.denominator * stepwise->step.denominator * desired->denominator;
	step *= stepwise->min.denominator * stepwise->max.denominator * desired->denominator;
	goal *= stepwise->min.denominator * stepwise->max.denominator * stepwise->step.denominator;

	/* Saturate the desired value to the min/max supported */
	goal = CLAMP(goal, min, max);

	/* Compute a numerator and denominator */
	match->numerator = min + DIV_ROUND_CLOSEST(goal - min, step) * step;
	match->denominator = stepwise->min.denominator * stepwise->max.denominator *
			     stepwise->step.denominator * desired->denominator;
}

/**
 * @brief Find the closest match to a frame interval value within a video device.
 *
 * To compute the closest match, fill @p match with the following fields:
 *
 * - @c match->format to the @struct video_format of interest.
 * - @c match->type to @ref VIDEO_FRMIVAL_TYPE_DISCRETE.
 * - @c match->discrete to the desired frame interval.
 *
 * The result will be loaded into @p match, with the following fields set:
 *
 * - @c match->discrete to the value of the closest frame interval.
 * - @c match->index to the index of the closest frame interval.
 *
 * @param dev Video device to query.
 * @param ep Video endpoint ID to query.
 * @param match Frame interval enumerator with the query, and loaded with the result.
 */
static inline void video_closest_frmival(const struct device *dev, enum video_endpoint_id ep,
					 struct video_frmival_enum *match)
{
	uint64_t best_diff_nsec = INT32_MAX;
	struct video_frmival desired = match->discrete;
	struct video_frmival_enum fie = {.format = match->format};

	__ASSERT(match->type != VIDEO_FRMIVAL_TYPE_STEPWISE,
		 "cannot find range matching the range, only a value matching the range");

	while (video_enum_frmival(dev, ep, &fie) == 0) {
		struct video_frmival tmp = {0};
		uint64_t diff_nsec = 0, a, b;

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			tmp = fie.discrete;
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			video_closest_frmival_stepwise(&fie.stepwise, &desired, &tmp);
			break;
		default:
			__ASSERT(false, "invalid answer from the queried video device");
		}

		a = video_frmival_nsec(&desired);
		b = video_frmival_nsec(&tmp);
		diff_nsec = a > b ? a - b : b - a;
		if (diff_nsec < best_diff_nsec) {
			best_diff_nsec = diff_nsec;
			memcpy(&match->discrete, &tmp, sizeof(tmp));

			/* The video_enum_frmival() function will increment fie.index every time.
			 * Compensate for it to get the current index, not the next index.
			 */
			match->index = fie.index - 1;
		}
	}
}

#endif /* ZEPHYR_INCLUDE_VIDEO_COMMON_H */

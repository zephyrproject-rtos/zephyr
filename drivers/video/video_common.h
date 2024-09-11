/*
 * Copyright (c) 2024, tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_COMMON_H
#define ZEPHYR_INCLUDE_VIDEO_COMMON_H

#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/video.h>

/**
 * @file
 * @brief Internal utilities to facilitate implementation of video driver APIs
 *
 * This permits to deduplicate code through all drivers reducing code size,
 * and reduce the amount of boilerplate needed for each driver.
 */

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
static inline int video_get_format_index(struct video_format_cap *fmts, struct video_format *fmt,
					 int *idx)
{
	for (int i = 0; fmts[i].pixelformat != 0; i++) {
		if (fmts[i].width_min == fmt->width && fmts[i].height_min == fmt->height &&
		    fmts[i].pixelformat == fmt->pixelformat) {
			*idx = i;
			return 0;
		}
	}
	return -ENOENT;
}

/**
 * @brief Compute the difference between two frame intervals
 *
 * @param ap First frame interval.
 * @param bp Second frame interval.
 *
 * @return The signed difference in microsecond between the two frame intervals.
 */
static inline int64_t video_diff_frmival_usec(struct video_frmival *ap, struct video_frmival *bp)
{
	struct video_frmival a = *ap;
	struct video_frmival b = *bp;

	if (a.denominator != b.denominator) {
		a.numerator *= b.denominator;
		a.denominator *= b.denominator;
		b.numerator *= a.denominator;
		b.denominator *= a.denominator;
	}

	/* Return the difference in microseconds */
	return DIV_ROUND_CLOSEST((int64_t)USEC_PER_SEC * a.numerator, a.denominator) -
	       DIV_ROUND_CLOSEST((int64_t)USEC_PER_SEC * b.numerator, b.denominator);
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
						  struct video_frmival *best_match)
{
	uint32_t min = stepwise->min.numerator;
	uint32_t max = stepwise->max.numerator;
	uint32_t step = stepwise->step.numerator;
	uint32_t desi = desired->numerator;

	min *= stepwise->max.denominator * stepwise->step.denominator * desired->denominator;
	max *= stepwise->min.denominator * stepwise->step.denominator * desired->denominator;
	step *= stepwise->min.denominator * stepwise->max.denominator * desired->denominator;
	goal *= stepwise->min.denominator * stepwise->max.denominator * stepwise->step.denominator;

	best_match->denominator = stepwise->min.denominator * stepwise->max.denominator *
				  stepwise->step.denominator * desired->denominator;
	best_match->numerator = min + DIV_ROUND_CLOSEST(goal - min, step) * step;
}

/**
 * @brief Find the closest match to a frame interval value within a video device.
 *
 * @param dev Video device to search within.
 * @param desired Frame interval for which find a close match.
 * @param best_fie Frame interval enumerator pointing at the closest match.
 */
static inline void video_closest_frmival(const struct device *dev,
					 const struct video_frmival *desired,
					 struct video_frmival_enum *best_fie)
{
	int32_t best_diff_usec = INT32_MAX;
	struct video_frmival_enum fie = {.format = best_fie->format};
	int ret;

	while (video_enum_frmival(dev, ep, &fie) == 0) {
		struct video_frmival best_stepwise = {0};
		int32_t diff_usec;

		switch (fie->type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			diff_usec = video_diff_frmival_usec(desired, &fie->discrete);
			break;
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			video_closest_frmival_stepwise(&fie->stepwise, desired, &best_stepwise);
			diff_usec = video_diff_frmival_usec(desired, &best_stepwise);
			break;
		default:
			__ASSERT(false, "invalid video device")
			return;
		}

		if (ABS(diff_usec) < best_diff_usec) {
			best_diff_usec = diff_usec;
			memcpy(best_fie, &fie, sizeof(*best_fie));
		}
	}
}

#endif /* ZEPHYR_INCLUDE_VIDEO_COMMON_H */

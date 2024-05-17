/*
 * Copyright (c) 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_MATH_INTERPOLATION_H_
#define ZEPHYR_INCLUDE_ZEPHYR_MATH_INTERPOLATION_H_

#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Provide linear interpolation functions
 */

/**
 * @brief Perform a linear interpolation across an arbitrary curve
 *
 * @note Result rounding occurs away from 0, e.g:
 *       1.5 -> 2, -5.5 -> -6
 *
 * @param x_axis Ascending list of X co-ordinates for @a y_axis data points
 * @param y_axis Y co-ordinates for each X data point
 * @param len Length of the @a x_axis and @a y_axis arrays
 * @param x X co-ordinate to lookup
 *
 * @retval y_axis[0] if x < x_axis[0]
 * @retval y_axis[len - 1] if x > x_axis[len - 1]
 * @retval int32_t Linear interpolation between the two nearest @a y_axis values.
 */
static inline int32_t linear_interpolate(const int32_t *x_axis, const int32_t *y_axis, uint8_t len,
					 int32_t x)
{
	float rise, run, slope;
	int32_t x_shifted;
	uint8_t idx_low = 0;

	/* Handle out of bounds values */
	if (x <= x_axis[0]) {
		return y_axis[0];
	} else if (x >= x_axis[len - 1]) {
		return y_axis[len - 1];
	}

	/* Find the lower x axis bucket */
	while (x >= x_axis[idx_low + 1]) {
		idx_low++;
	}

	/* Shift input to origin */
	x_shifted = x - x_axis[idx_low];
	if (x_shifted == 0) {
		return y_axis[idx_low];
	}

	/* Local slope */
	rise = y_axis[idx_low + 1] - y_axis[idx_low];
	run = x_axis[idx_low + 1] - x_axis[idx_low];
	slope = rise / run;

	/* Apply slope, undo origin shift and round */
	return roundf(y_axis[idx_low] + (slope * x_shifted));
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_MATH_INTERPOLATION_H_ */

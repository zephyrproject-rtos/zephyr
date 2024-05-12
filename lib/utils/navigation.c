/*
 * Copyright 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/data/navigation.h>

#define NAV_BEARING_MAX   360000
#define NAV_BEARING_MIN   0
#define NAV_LATITUDE_MAX  90000000000
#define NAV_LATITUDE_MIN  -90000000000
#define NAV_LONGITUDE_MAX 180000000000
#define NAV_LONGITUDE_MIN -180000000000

/*
 * Input: 1/65535
 * Output: millidegrees
 * Mathematic notation: (0.861 * x)
 * Accuracy: +-73 millidegrees for input range [0 .. 13107]
 */
static uint16_t nav_atan_linear(uint16_t x)
{
	uint32_t a;

	a = x;
	a <<= 16;
	a /= 76116;
	return a;
}

/*
 * Input: 1/65535
 * Output: millidegrees
 * Mathematic notation: (0.6995 * x) + ((0.00000429 * x) * (65535 - x)) - 880
 * Accuracy: +-86 millidegrees for input range [13107 .. 65535]
 *
 * Inspired by IEEE SIGNAL PROCESSING MAGAZINE, May 2006, page 109, equation 7
 */
static uint16_t nav_atan_poly(uint16_t x)
{
	uint32_t a;
	uint32_t b;

	a = x;
	a <<= 16;
	a /= 93690;

	b = x;
	b <<= 16;
	b /= 233100;
	b *= 65535 - x;
	b >>= 16;

	return a + b - 880;
}

/*
 * Input: 1/65535
 * Output: millidegrees
 * Accuracy: +-86 millidegrees
 */
static uint16_t nav_atan(uint16_t x)
{
	if (x == 0) {
		return 0;
	}

	if (x < 13108) {
		return nav_atan_linear(x);
	}

	if (x < 65535) {
		return nav_atan_poly(x);
	}

	return 45000;
}

static uint8_t nav_atan2_quadrant(const int64_t *y, const int64_t *x)
{
	if (*y >= 0) {
		if (*x >= 0) {
			return 0;
		}

		return 1;
	}

	if (*x < 0) {
		return 2;
	}

	return 3;
}

static void nav_atan2_abs(int64_t *x)
{
	*x = *x < 0 ? -(*x) : *x;
}

static bool nav_atan2_swap(int64_t *y, int64_t *x)
{
	int64_t cache;

	if (*y < *x) {
		cache = *x;
		*x = *y;
		*y = cache;
		return true;
	}

	return false;
}

static void nav_atan2_quantize(int64_t *y, int64_t *x)
{
	if (*y == 0) {
		return;
	}

	while (*y >= 4294967296) {
		*x >>= 1;
		*y >>= 1;
	}

	while (*y < 2147483648) {
		*x <<= 1;
		*y <<= 1;
	}
}

static uint16_t nav_atan2_divide(const int64_t *y, const int64_t *x)
{
	uint32_t numerator;
	uint32_t denominator;

	numerator = (uint32_t)*x;
	denominator = (uint32_t)*y;

	if (numerator == denominator) {
		return UINT16_MAX;
	}

	denominator >>= 16;

	if (denominator == 0) {
		return UINT16_MAX;
	}

	return numerator / denominator;
}

static uint32_t nav_atan2_translate(uint16_t angle, uint8_t quadrant, bool swapped)
{
	uint32_t udeg;

	udeg = 90000;
	udeg *= quadrant;

	if (quadrant % 2) {
		udeg += swapped ? 90000 - angle : angle;
	} else {
		udeg += swapped ? angle : 90000 - angle;
	}

	return udeg;
}

static uint32_t nav_atan2(int64_t x, int64_t y)
{
	uint8_t quadrant;
	bool swapped;
	uint16_t angle;

	quadrant = nav_atan2_quadrant(&x, &y);
	nav_atan2_abs(&x);
	nav_atan2_abs(&y);
	swapped = nav_atan2_swap(&x, &y);
	nav_atan2_quantize(&x, &y);
	angle = nav_atan2_divide(&x, &y);
	angle = nav_atan(angle);
	return nav_atan2_translate(angle, quadrant, swapped);
}

static void nav_delta_latitude(int64_t *delta, const int64_t *to, const int64_t *from)
{
	*delta = *to - *from;
}

static void nav_delta_longitude(int64_t *delta, const int64_t *to, const int64_t *from)
{
	/* Longitudes wrap at +-180 degrees */
	if ((*to >= NAV_LATITUDE_MAX) && (*from <= NAV_LATITUDE_MIN)) {
		*delta = (*to - NAV_LONGITUDE_MAX) - (*from + NAV_LONGITUDE_MAX);
	} else if ((*to <= NAV_LATITUDE_MIN) && (*from >= NAV_LATITUDE_MAX)) {
		*delta = (*to + NAV_LONGITUDE_MAX) - (*from - NAV_LONGITUDE_MAX);
	} else {
		*delta = *to - *from;
	}
}

static bool nav_validate_data(const struct navigation_data *data)
{
	return (data->latitude <= NAV_LATITUDE_MAX) &&
	       (data->latitude >= NAV_LATITUDE_MIN) &&
	       (data->longitude <= NAV_LONGITUDE_MAX) &&
	       (data->longitude >= NAV_LONGITUDE_MIN);
}

int navigation_bearing(uint32_t *bearing, const struct navigation_data *from,
		       const struct navigation_data *to)
{
	int64_t latitude;
	int64_t longitude;

	if (!nav_validate_data(from) || !nav_validate_data(to)) {
		return -EINVAL;
	}

	nav_delta_latitude(&latitude, &to->latitude, &from->latitude);
	nav_delta_longitude(&longitude, &to->longitude, &from->longitude);
	*bearing = nav_atan2(longitude, latitude);
	return 0;
}

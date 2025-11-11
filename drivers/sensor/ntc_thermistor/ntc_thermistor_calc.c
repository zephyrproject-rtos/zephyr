/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include "ntc_thermistor.h"

/**
 * fixp_linear_interpolate() - interpolates a value from two known points
 *
 * @x0: x value of point 0
 * @y0: y value of point 0
 * @x1: x value of point 1
 * @y1: y value of point 1
 * @x: the linear interpolant
 */
static int ntc_fixp_linear_interpolate(int x0, int y0, int x1, int y1, int x)
{
	if (y0 == y1 || x == x0) {
		return y0;
	}
	if (x1 == x0 || x == x1) {
		return y1;
	}

	return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

/**
 * Finds indices where ohm falls between.
 *
 * @param ohm key value search is looking for
 * @param i_low return Lower interval index value
 * @param i_high return Higher interval index value
 */
static void ntc_lookup_comp(const struct ntc_type *type, unsigned int ohm, int *i_low, int *i_high)
{
	int low = 0;
	int high = type->n_comp - 1;

	if (ohm > type->comp[low].ohm) {
		high = low;
	} else if (ohm < type->comp[high].ohm) {
		low = high;
	}

	while (high - low > 1) {
		int mid = (low + high) / 2;

		if (ohm > type->comp[mid].ohm) {
			high = mid;
		} else {
			low = mid;
		}
	}

	*i_low = low;
	*i_high = high;
}

uint32_t ntc_get_ohm_of_thermistor(const struct ntc_config *cfg, int sample_value,
				   int sample_value_max)
{
	uint32_t ohm;

	if (sample_value <= 0) {
		return cfg->connected_positive ? INT_MAX : 0;
	}

	if (sample_value >= sample_value_max) {
		return cfg->connected_positive ? 0 : INT_MAX;
	}

	if (cfg->connected_positive) {
		ohm = cfg->pulldown_ohm * (sample_value_max - sample_value) / sample_value;
	} else {
		ohm = cfg->pullup_ohm * sample_value / (sample_value_max - sample_value);
	}

	return ohm;
}

int32_t ntc_get_temp_mc(const struct ntc_type *type, unsigned int ohm)
{
	int low, high;
	int temp;

	ntc_lookup_comp(type, ohm, &low, &high);
	/*
	 * First multiplying the table temperatures with 1000 to get to
	 * millicentigrades (which is what we want) and then interpolating
	 * will give the best precision.
	 */
	temp = ntc_fixp_linear_interpolate(type->comp[low].ohm, type->comp[low].temp_c * 1000,
					   type->comp[high].ohm, type->comp[high].temp_c * 1000,
					   ohm);
	return temp;
}

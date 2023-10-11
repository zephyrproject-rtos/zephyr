/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <stdlib.h>
#include <zephyr/devicetree.h>
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
 * ntc_lookup_comp() - Finds indicies where ohm falls between
 *
 * @ohm: key value search is looking for
 * @i_low: return Lower interval index value
 * @i_high: return Higher interval index value
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

/**
 * ntc_get_ohm_of_thermistor() - Calculate the resistance read from NTC Thermistor
 *
 * @cfg: NTC Thermistor configuration
 * @sample_mv: Measured voltage in mV
 */
uint32_t ntc_get_ohm_of_thermistor(const struct ntc_config *cfg, int sample_mv)
{
	int pullup_mv = cfg->pullup_uv / 1000;
	uint32_t ohm;

	if (sample_mv <= 0) {
		return cfg->connected_positive ? INT_MAX : 0;
	}

	if (sample_mv >= pullup_mv) {
		return cfg->connected_positive ? 0 : INT_MAX;
	}

	if (cfg->connected_positive) {
		ohm = cfg->pulldown_ohm * (pullup_mv - sample_mv) / sample_mv;
	} else {
		ohm = cfg->pullup_ohm * sample_mv / (pullup_mv - sample_mv);
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

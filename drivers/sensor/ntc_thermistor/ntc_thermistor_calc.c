/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
 * ntc_compensation_compare_ohm() - Helper comparison function for bsearch
 *
 * Ohms are sorted in descending order, perform comparison to find
 * interval indexes where key falls between
 *
 * @type: Pointer to ntc_type table info
 * @key: Key value bsearch is looking for
 * @element: Array element bsearch is searching
 */
int ntc_compensation_compare_ohm(const struct ntc_type *type, const void *key, const void *element)
{
	int sgn = 0;
	const struct ntc_compensation *ntc_key = key;
	const struct ntc_compensation *element_val = element;
	int element_idx = element_val - type->comp;

	if (ntc_key->ohm > element_val->ohm) {
		if (element_idx == 0) {
			sgn = 0;
		} else {
			sgn = -1;
		}
	} else if (ntc_key->ohm == element_val->ohm) {
		sgn = 0;
	} else if (ntc_key->ohm < element_val->ohm) {
		if (element_idx == (type->n_comp / 2) - 1) {
			sgn = 0;
		} else {
			if (element_idx != (type->n_comp / 2) - 1 &&
			    ntc_key->ohm > type->comp[element_idx + 1].ohm) {
				sgn = 0;
			} else {
				sgn = 1;
			}
		}
	}

	return sgn;
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
	const struct ntc_compensation *ptr;
	struct ntc_compensation search_ohm_key = {.ohm = ohm};

	ptr = bsearch(&search_ohm_key, type->comp, type->n_comp, sizeof(type->comp[0]),
		      type->ohm_cmp);
	if (ptr) {
		*i_low = ptr - type->comp;
		*i_high = *i_low + 1;
	} else {
		*i_low = 0;
		*i_high = 0;
	}
}

/**
 * ntc_get_ohm_of_thermistor() - Calculate the resistance read from NTC Thermistor
 *
 * @cfg: NTC Thermistor configuration
 * @max_adc: Max ADC value
 * @raw_adc: Raw ADC value read
 */
uint32_t ntc_get_ohm_of_thermistor(const struct ntc_config *cfg, uint32_t max_adc, int16_t raw_adc)
{
	uint32_t ohm;

	if (cfg->connected_positive) {
		ohm = cfg->pulldown_ohm * max_adc / (raw_adc - 1);
	} else {
		ohm = cfg->pullup_ohm * (raw_adc - 1) / max_adc;
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

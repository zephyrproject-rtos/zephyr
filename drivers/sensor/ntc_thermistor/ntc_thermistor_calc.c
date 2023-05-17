/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "ntc_thermistor.h"

LOG_MODULE_DECLARE(NTC_THERMISTOR, CONFIG_SENSOR_LOG_LEVEL);

/**
 * fixp_linear_interpolate() - interpolates a value from two known points
 *
 * @x0: x value of point 0 (Ohms)
 * @y0: y value of point 0 (Celsius)
 * @x1: x value of point 1 (Ohms)
 * @y1: y value of point 1 (Celsius)
 * @x: the linear interpolant (Ohms)
 *
 * @return: interpolated value (Celsius)
 */
static int ntc_fixp_linear_interpolate(uint32_t x0, int32_t y0, uint32_t x1, int32_t y1, uint32_t x)
{
	if (y0 == y1 || x == x0) {
		return y0;
	}
	if (x1 == x0 || x == x1) {
		return y1;
	}

	return (int32_t)(y0 + ((y1 - y0) * ((int64_t)x - x0) / ((int64_t)x1 - x0)));
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
	unsigned int element_idx = (unsigned int)(element_val - type->comp);

	if (ntc_key->ohm > element_val->ohm) {
		if (element_idx == 0) {
			sgn = 0;
		} else {
			sgn = -1;
		}
	} else if (ntc_key->ohm == element_val->ohm) {
		sgn = 0;
	} else {
		if (element_idx == type->n_comp - 1) {
			sgn = 0;
		} else {
			if (ntc_key->ohm > type->comp[element_idx + 1].ohm) {
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
static void ntc_lookup_comp(const struct ntc_type *type, uint32_t ohm, unsigned int *i_low,
			    unsigned int *i_high)
{
	const struct ntc_compensation *ptr;
	struct ntc_compensation search_ohm_key = {.ohm = ohm};

	ptr = bsearch(&search_ohm_key, type->comp, type->n_comp, sizeof(type->comp[0]),
		      type->ohm_cmp);
	__ASSERT_NO_MSG(ptr != NULL);

	*i_low = (unsigned int)(ptr - type->comp);
	if (*i_low == type->n_comp - 1) {
		--*i_low;
	}
	*i_high = *i_low + 1;
}

/**
 * ntc_get_ohm_of_thermistor() - Calculate the resistance read from NTC Thermistor
 *
 * @cfg: NTC Thermistor configuration
 * @vout: Measured voltage divider output voltage in millivolts
 */
uint32_t ntc_get_ohm_of_thermistor(const struct ntc_config *cfg, uint32_t vout)
{
	const uint32_t vin = cfg->pullup_uv / 1000;
	uint32_t ohm;

	if (vout > vin) {
		LOG_WRN("Measured Vout is higher than Vin ('pullup_uv' or ADC reference voltage "
			"needs to be fixed in devicetree)");
		vout = vin;
	}

	/* Vout = (R2 / (R1 + R2)) * Vin */
	if (cfg->connected_positive) {
		if (!vout) {
			/* Edge case to avoid dividing by 0 (R1 headed to infinity) */
			ohm = UINT32_MAX;
		} else {
			ohm = ((cfg->pulldown_ohm * vin) / vout) - cfg->pulldown_ohm;
		}
	} else {
		if (vout == vin) {
			/* Edge case to avoid dividing by 0 (R2 headed to infinity) */
			ohm = UINT32_MAX;
		} else {
			ohm = (vout * cfg->pullup_ohm) / (vin - vout);
		}
	}

	return ohm;
}

int32_t ntc_get_temp_mc(const struct ntc_type *type, uint32_t ohm)
{
	const int32_t min_temp = type->comp[0].temp_c * 1000;
	const int32_t max_temp = type->comp[type->n_comp - 1].temp_c * 1000;
	unsigned int low, high;
	int32_t temp;

	ntc_lookup_comp(type, ohm, &low, &high);
	/*
	 * First multiplying the table temperatures with 1000 to get to
	 * millicentigrades (which is what we want) and then interpolating
	 * will give the best precision.
	 */
	temp = ntc_fixp_linear_interpolate(type->comp[low].ohm, type->comp[low].temp_c * 1000,
					   type->comp[high].ohm, type->comp[high].temp_c * 1000,
					   ohm);

	return CLAMP(temp, min_temp, max_temp);
}

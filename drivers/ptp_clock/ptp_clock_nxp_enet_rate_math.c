/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ptp_clock_nxp_enet_rate_math.h"

#include <errno.h>
#include <math.h>
#include <stdbool.h>

/*
 * Try one correction period for a given INC_CORR - INC distance (delta). The best pair found
 * so far is kept in the output arguments. ATCOR = period - 1 has to be in [1, cor_max]:
 * ATCOR = 0 turns the correction off, so a period of 1 cannot be programmed.
 */
static bool try_period(int delta, uint32_t period, double frac, uint32_t cor_max, double *best_err,
		       int *out_delta, uint32_t *out_cor)
{
	double err;

	if (period < 2U || period - 1U > cor_max) {
		return false;
	}

	err = fabs((double)delta / (double)period - frac);
	if (err >= *best_err) {
		return false;
	}

	*best_err = err;
	*out_delta = delta;
	*out_cor = period - 1U;

	return true;
}

/*
 * For a given delta, delta / period falls as the period grows. The best period is next to
 * delta / frac, but not always the rounded one: the point where the two neighbours are equally
 * close is not at the .5 mark. Both neighbours are tried.
 */
static bool find_best_delta_cor(double frac, int delta_max, uint32_t cor_max, int *out_delta,
				uint32_t *out_cor)
{
	double best_err = HUGE_VAL;
	bool found = false;

	for (int delta = 1; delta <= delta_max; delta++) {
		double period_exact = (double)delta / frac;
		uint32_t period_floor;

		/* No period fits in ATCOR, and the conversion below would overflow */
		if (period_exact > (double)cor_max) {
			continue;
		}

		period_floor = (uint32_t)floor(period_exact);

		found |= try_period(delta, period_floor, frac, cor_max, &best_err, out_delta,
				    out_cor);
		found |= try_period(delta, period_floor + 1U, frac, cor_max, &best_err, out_delta,
				    out_cor);
	}

	return found;
}

int ptp_clock_nxp_enet_find_correction(int inc, double target_ns, int inc_corr_max,
				       uint32_t cor_max, int *out_inc_corr, uint32_t *out_cor)
{
	double frac = target_ns - (double)inc;
	int delta_max;
	int delta = 0;
	uint32_t cor = 0U;

	if (frac == 0.0) {
		*out_inc_corr = inc;
		*out_cor = 0U;
		return 0;
	}

	/* A target above inc needs INC_CORR above it, one under inc needs INC_CORR below it */
	delta_max = (frac > 0.0) ? (inc_corr_max - inc) : inc;
	if (delta_max < 1) {
		return -EINVAL;
	}

	if (!find_best_delta_cor(fabs(frac), delta_max, cor_max, &delta, &cor)) {
		/* The wanted correction is too small for any period that fits in ATCOR */
		*out_inc_corr = inc;
		*out_cor = 0U;
		return 0;
	}

	*out_inc_corr = (frac > 0.0) ? (inc + delta) : (inc - delta);
	*out_cor = cor;

	return 0;
}

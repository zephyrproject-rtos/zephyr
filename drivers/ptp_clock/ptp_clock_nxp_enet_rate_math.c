/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ptp_clock_nxp_enet_rate_math.h"

#include <errno.h>
#include <float.h>
#include <stdbool.h>

/* Kept local so that the driver does not need the math library */
static double abs_double(double value)
{
	return (value < 0.0) ? -value : value;
}

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

	err = abs_double((double)delta / (double)period - frac);
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
 *
 * The smallest delta comes first and the search stops at the first pair within the tolerance,
 * since a smaller delta is a smaller step of the time on a corrected tick.
 */
static bool find_best_delta_cor(double frac, double tolerance, int delta_max, uint32_t cor_max,
				int *out_delta, uint32_t *out_cor)
{
	double best_err = DBL_MAX;
	bool found = false;

	for (int delta = 1; delta <= delta_max; delta++) {
		double period_exact = (double)delta / frac;
		uint32_t period_floor;

		/* A larger delta needs an even longer period, so no later one fits in ATCOR */
		if (period_exact > (double)cor_max) {
			break;
		}

		/* period_exact is positive and fits in 32 bits, so the conversion is its floor */
		period_floor = (uint32_t)period_exact;

		if (try_period(delta, period_floor, frac, cor_max, &best_err, out_delta, out_cor)) {
			found = true;
		}
		if (try_period(delta, period_floor + 1U, frac, cor_max, &best_err, out_delta,
			       out_cor)) {
			found = true;
		}

		if (found && best_err <= tolerance) {
			break;
		}
	}

	return found;
}

int ptp_clock_nxp_enet_find_correction(int inc, double frac_ns, double tolerance_ns,
				       int inc_corr_max, uint32_t cor_max, int *out_inc_corr,
				       uint32_t *out_cor)
{
	double frac_abs = abs_double(frac_ns);
	int delta_max;
	int delta = 0;
	uint32_t cor = 0U;

	if (frac_ns == 0.0) {
		*out_inc_corr = inc;
		*out_cor = 0U;
		return 0;
	}

	/* A target above inc needs INC_CORR above it, one under inc needs INC_CORR below it */
	delta_max = (frac_ns > 0.0) ? (inc_corr_max - inc) : inc;

	/*
	 * The shortest period is 2 ticks, so delta_max / 2 is the largest shift that can be
	 * reached. Written so that a value that is not a number is rejected too.
	 */
	if ((delta_max < 1) || !(frac_abs <= (double)delta_max / 2.0)) {
		return -EINVAL;
	}

	if (!find_best_delta_cor(frac_abs, tolerance_ns, delta_max, cor_max, &delta, &cor)) {
		/* The wanted shift is too small for any period that fits in ATCOR */
		*out_inc_corr = inc;
		*out_cor = 0U;
		return 0;
	}

	*out_inc_corr = (frac_ns > 0.0) ? (inc + delta) : (inc - delta);
	*out_cor = cor;

	return 0;
}

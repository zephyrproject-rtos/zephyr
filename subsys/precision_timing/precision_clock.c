/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <math.h>
#include <stddef.h>

#include <zephyr/precision_timing/precision_clock.h>

int precision_clock_read(const struct precision_clock *precision_clk, precision_time_t *time_ns)
{
	return precision_clk->api->read(precision_clk, time_ns);
}

int precision_clock_set(const struct precision_clock *precision_clk, precision_time_t time_ns)
{
	return precision_clk->api->set(precision_clk, time_ns);
}

int precision_clock_adjust_phase(const struct precision_clock *precision_clk,
				 precision_time_t phase_ns)
{
	return precision_clk->api->adjust_phase(precision_clk, phase_ns);
}

int precision_clock_ppb_to_scaled_ppm(double ppb, int64_t *scaled_ppm)
{
	double value;

	if (scaled_ppm == NULL) {
		return -EINVAL;
	}

	if (!isfinite(ppb)) {
		return -ERANGE;
	}

	value = ppb * PRECISION_CLOCK_SCALED_PPM_ONE / 1000.0;
	if (!isfinite(value) || value < (double)INT64_MIN ||
	    value >= -((double)INT64_MIN)) {
		return -ERANGE;
	}

	*scaled_ppm = (int64_t)value;

	return 0;
}

int precision_clock_adjust_rate(const struct precision_clock *precision_clk, int64_t scaled_ppm)
{
	return precision_clk->api->adjust_rate(precision_clk, scaled_ppm);
}

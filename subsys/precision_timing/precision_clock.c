/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/precision_timing/precision_clock.h>

int precision_clock_read(const struct precision_clock *precision_clk, precision_time_t *time_ns)
{
	if (precision_clk == NULL || time_ns == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->read == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->read(precision_clk, time_ns);
}

int precision_clock_set(const struct precision_clock *precision_clk, precision_time_t time_ns)
{
	if (precision_clk == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->set == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->set(precision_clk, time_ns);
}

int precision_clock_adjust_phase(const struct precision_clock *precision_clk,
				 precision_time_t phase_ns)
{
	if (precision_clk == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->adjust_phase == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->adjust_phase(precision_clk, phase_ns);
}

int precision_clock_adjust_rate(const struct precision_clock *precision_clk, double rate_ratio)
{
	if (precision_clk == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->adjust_rate == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->adjust_rate(precision_clk, rate_ratio);
}

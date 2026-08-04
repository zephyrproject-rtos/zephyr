/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/precision_timing/precision_clock.h>

int precision_clock_read(const struct precision_clock *precision_clk,
			 struct precision_time_point *tp)
{
	int ret;

	if (precision_clk == NULL || tp == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->read == NULL) {
		return -ENOTSUP;
	}

	ret = precision_clk->api->read(precision_clk, tp);
	if (ret == 0 && tp->domain.type == PRECISION_TIME_DOMAIN_INVALID) {
		tp->domain = precision_clk->domain;
	}

	return ret;
}

int precision_clock_set(const struct precision_clock *precision_clk,
			const struct precision_time_point *tp)
{
	if (precision_clk == NULL || tp == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (!precision_time_domain_equal(&precision_clk->domain, &tp->domain)) {
		return -EINVAL;
	}

	if (precision_clk->api->set == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->set(precision_clk, tp);
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

int precision_clock_adjust_rate(const struct precision_clock *precision_clk, int32_t rate_ppb)
{
	if (precision_clk == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->adjust_rate == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->adjust_rate(precision_clk, rate_ppb);
}

int precision_clock_get_caps(const struct precision_clock *precision_clk,
			     struct precision_clock_caps *caps)
{
	if (precision_clk == NULL || caps == NULL || precision_clk->api == NULL) {
		return -EINVAL;
	}

	if (precision_clk->api->get_caps == NULL) {
		return -ENOTSUP;
	}

	return precision_clk->api->get_caps(precision_clk, caps);
}

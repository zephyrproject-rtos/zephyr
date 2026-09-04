/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>
#include <zephyr/sys/clock.h>

static const struct ptp_clock_driver_api *ptp_api(const struct precision_clock *precision_clk)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;

	return DEVICE_API_GET(ptp_clock, adapter->dev);
}

static int precision_clock_ptp_read(const struct precision_clock *precision_clk,
				    precision_time_t *time_ns)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct ptp_clock_driver_api *api = ptp_api(precision_clk);
	struct net_ptp_time ptp_time;
	uint64_t seconds_limit = PRECISION_TIME_MAX / NSEC_PER_SEC;
	uint32_t nanoseconds_limit = PRECISION_TIME_MAX % NSEC_PER_SEC;
	int ret;

	ret = api->get(adapter->dev, &ptp_time);
	if (ret < 0) {
		return ret;
	}

	if (ptp_time.nanosecond >= NSEC_PER_SEC || ptp_time.second > seconds_limit ||
	    (ptp_time.second == seconds_limit && ptp_time.nanosecond > nanoseconds_limit)) {
		return -ERANGE;
	}

	*time_ns = (precision_time_t)ptp_time.second * NSEC_PER_SEC + ptp_time.nanosecond;

	return 0;
}

static int precision_clock_ptp_set(const struct precision_clock *precision_clk,
				   precision_time_t time_ns)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct ptp_clock_driver_api *api = ptp_api(precision_clk);
	struct net_ptp_time ptp_time;

	if (time_ns < 0) {
		return -ERANGE;
	}

	ptp_time.second = (uint64_t)(time_ns / NSEC_PER_SEC);
	ptp_time.nanosecond = (uint32_t)(time_ns % NSEC_PER_SEC);

	return api->set(adapter->dev, &ptp_time);
}

static int precision_clock_ptp_adjust_phase(const struct precision_clock *precision_clk,
					    precision_time_t phase_ns)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct ptp_clock_driver_api *api = ptp_api(precision_clk);

	if (phase_ns < INT_MIN || phase_ns > INT_MAX) {
		return -ERANGE;
	}

	return api->adjust(adapter->dev, (int)phase_ns);
}

static int precision_clock_ptp_adjust_rate(const struct precision_clock *precision_clk,
					   int64_t scaled_ppm)
{
	const struct precision_clock_ptp_adapter *adapter = precision_clk->data;
	const struct ptp_clock_driver_api *api = ptp_api(precision_clk);
	/* The existing PTP clock API expresses the adjustment as a rate ratio. */
	double rate_ratio =
		1.0 + (double)scaled_ppm / (1000000.0 * PRECISION_CLOCK_SCALED_PPM_ONE);

	return api->rate_adjust(adapter->dev, rate_ratio);
}

static const struct precision_clock_api precision_clock_ptp_api = {
	.read = precision_clock_ptp_read,
	.set = precision_clock_ptp_set,
	.adjust_phase = precision_clock_ptp_adjust_phase,
	.adjust_rate = precision_clock_ptp_adjust_rate,
};

int precision_clock_ptp_init(struct precision_clock_ptp_adapter *adapter, const struct device *dev)
{
	if (adapter == NULL || dev == NULL) {
		return -EINVAL;
	}

	adapter->dev = dev;
	adapter->precision_clk.api = &precision_clock_ptp_api;
	adapter->precision_clk.data = adapter;

	return 0;
}

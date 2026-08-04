/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/sys/check.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>

/* Keep the rate ratio passed to ptp_clock_rate_adjust() strictly positive. */
#define PRECISION_CLOCK_PTP_MIN_RATE_PPB (-999999999)

static int precision_clock_ptp_read(const struct precision_clock *precision_clk,
				    struct precision_time_point *tp)
{
	struct precision_clock_ptp_adapter *adapter =
		(struct precision_clock_ptp_adapter *)precision_clk->adapter;
	const struct ptp_clock_driver_api *api;
	struct net_ptp_time ptp_time;
	int ret;

	if (adapter == NULL || adapter->ptp_clock == NULL) {
		return -EINVAL;
	}

	if (!(adapter->caps.flags & PRECISION_CLOCK_CAP_READ)) {
		return -ENOTSUP;
	}

	api = DEVICE_API_GET(ptp_clock, adapter->ptp_clock);
	if (api == NULL || api->get == NULL) {
		return -ENOTSUP;
	}

	ret = api->get(adapter->ptp_clock, &ptp_time);
	if (ret < 0) {
		return ret;
	}

	ret = precision_time_from_u64_sec_nsec(ptp_time.second, ptp_time.nanosecond, &tp->time);
	if (ret < 0) {
		return ret;
	}

	tp->domain = precision_clk->domain;

	return 0;
}

static int precision_clock_ptp_set(const struct precision_clock *precision_clk,
				   const struct precision_time_point *tp)
{
	struct precision_clock_ptp_adapter *adapter =
		(struct precision_clock_ptp_adapter *)precision_clk->adapter;
	const struct ptp_clock_driver_api *api;
	struct net_ptp_time ptp_time;
	int ret;

	if (adapter == NULL || adapter->ptp_clock == NULL) {
		return -EINVAL;
	}

	if (!(adapter->caps.flags & PRECISION_CLOCK_CAP_SET)) {
		return -ENOTSUP;
	}

	api = DEVICE_API_GET(ptp_clock, adapter->ptp_clock);
	if (api == NULL || api->set == NULL) {
		return -ENOTSUP;
	}

	ret = precision_time_to_u64_sec_nsec(tp->time, &ptp_time.second, &ptp_time.nanosecond);
	if (ret < 0) {
		return ret;
	}

	return api->set(adapter->ptp_clock, &ptp_time);
}

static int precision_clock_ptp_adjust_phase(const struct precision_clock *precision_clk,
					    precision_time_t phase_ns)
{
	struct precision_clock_ptp_adapter *adapter =
		(struct precision_clock_ptp_adapter *)precision_clk->adapter;
	const struct ptp_clock_driver_api *api;

	if (adapter == NULL || adapter->ptp_clock == NULL) {
		return -EINVAL;
	}

	if (!(adapter->caps.flags & PRECISION_CLOCK_CAP_ADJUST_PHASE)) {
		return -ENOTSUP;
	}

	if (phase_ns < INT_MIN || phase_ns > INT_MAX) {
		return -ERANGE;
	}

	if (adapter->caps.max_phase_adjust_ns > 0 &&
	    (phase_ns < -adapter->caps.max_phase_adjust_ns ||
	     phase_ns > adapter->caps.max_phase_adjust_ns)) {
		return -ERANGE;
	}

	api = DEVICE_API_GET(ptp_clock, adapter->ptp_clock);
	if (api == NULL || api->adjust == NULL) {
		return -ENOTSUP;
	}

	return api->adjust(adapter->ptp_clock, (int)phase_ns);
}

static int precision_clock_ptp_adjust_rate(const struct precision_clock *precision_clk,
					   int32_t rate_ppb)
{
	struct precision_clock_ptp_adapter *adapter =
		(struct precision_clock_ptp_adapter *)precision_clk->adapter;
	const struct ptp_clock_driver_api *api;
	double ratio;

	if (adapter == NULL || adapter->ptp_clock == NULL) {
		return -EINVAL;
	}

	if (!(adapter->caps.flags & PRECISION_CLOCK_CAP_ADJUST_RATE)) {
		return -ENOTSUP;
	}

	if (rate_ppb < adapter->caps.min_rate_ppb || rate_ppb > adapter->caps.max_rate_ppb) {
		return -ERANGE;
	}

	api = DEVICE_API_GET(ptp_clock, adapter->ptp_clock);
	if (api == NULL || api->rate_adjust == NULL) {
		return -ENOTSUP;
	}

	ratio = 1.0 + ((double)rate_ppb / 1000000000.0);

	return api->rate_adjust(adapter->ptp_clock, ratio);
}

static int precision_clock_ptp_get_caps(const struct precision_clock *precision_clk,
					struct precision_clock_caps *caps)
{
	struct precision_clock_ptp_adapter *adapter =
		(struct precision_clock_ptp_adapter *)precision_clk->adapter;

	if (adapter == NULL || caps == NULL) {
		return -EINVAL;
	}

	*caps = adapter->caps;

	return 0;
}

static uint32_t precision_clock_ptp_cap_flags(uint32_t ptp_flags)
{
	uint32_t flags = 0U;

	if (ptp_flags & PTP_CLOCK_CAP_READ) {
		flags |= PRECISION_CLOCK_CAP_READ;
	}
	if (ptp_flags & PTP_CLOCK_CAP_SET) {
		flags |= PRECISION_CLOCK_CAP_SET;
	}
	if (ptp_flags & PTP_CLOCK_CAP_ADJUST) {
		flags |= PRECISION_CLOCK_CAP_ADJUST_PHASE;
	}
	if (ptp_flags & PTP_CLOCK_CAP_RATE_ADJUST) {
		flags |= PRECISION_CLOCK_CAP_ADJUST_RATE;
	}

	return flags;
}

/*
 * Derive capabilities for a driver that does not implement the optional
 * capability callback. The set, get, adjust, and rate_adjust callbacks are
 * mandatory in the PTP clock driver API, so a non-null callback is the same
 * guarantee that ptp_clock_set(), ptp_clock_get(), ptp_clock_adjust(), and
 * ptp_clock_rate_adjust() already rely on. Rate adjustment limits stay generic
 * because a legacy driver cannot describe them. Phase adjustment is bounded by
 * the int argument accepted by ptp_clock_adjust().
 */
static uint32_t precision_clock_ptp_legacy_cap_flags(const struct ptp_clock_driver_api *api)
{
	uint32_t flags = 0U;

	if (api == NULL) {
		return flags;
	}

	if (api->get != NULL) {
		flags |= PRECISION_CLOCK_CAP_READ;
	}
	if (api->set != NULL) {
		flags |= PRECISION_CLOCK_CAP_SET;
	}
	if (api->adjust != NULL) {
		flags |= PRECISION_CLOCK_CAP_ADJUST_PHASE;
	}
	if (api->rate_adjust != NULL) {
		flags |= PRECISION_CLOCK_CAP_ADJUST_RATE;
	}

	return flags;
}

static const struct precision_clock_api precision_clock_ptp_api = {
	.read = precision_clock_ptp_read,
	.set = precision_clock_ptp_set,
	.adjust_phase = precision_clock_ptp_adjust_phase,
	.adjust_rate = precision_clock_ptp_adjust_rate,
	.get_caps = precision_clock_ptp_get_caps,
};

int precision_clock_ptp_init(struct precision_clock_ptp_adapter *adapter,
			     const struct device *ptp_clock, struct precision_time_domain domain)
{
	const struct ptp_clock_driver_api *api;
	struct ptp_clock_caps ptp_caps;
	struct precision_clock_caps caps;
	int ret;

	CHECKIF((adapter == NULL) || (ptp_clock == NULL)) {
		return -EINVAL;
	}

	api = DEVICE_API_GET(ptp_clock, ptp_clock);

	caps = (struct precision_clock_caps){
		.flags = precision_clock_ptp_legacy_cap_flags(api),
		.resolution_ns = 1,
		.max_phase_adjust_ns = 0,
		.min_rate_ppb = 0,
		.max_rate_ppb = 0,
	};

	if ((caps.flags & PRECISION_CLOCK_CAP_ADJUST_PHASE) != 0U) {
		caps.max_phase_adjust_ns = INT_MAX;
	}

	if ((caps.flags & PRECISION_CLOCK_CAP_ADJUST_RATE) != 0U) {
		/* A legacy driver does not report its rate range, so leave the
		 * upper bound to the driver while keeping the derived ratio valid.
		 */
		caps.min_rate_ppb = PRECISION_CLOCK_PTP_MIN_RATE_PPB;
		caps.max_rate_ppb = INT32_MAX;
	}

	if ((api != NULL) && (api->get_caps != NULL)) {
		ret = api->get_caps(ptp_clock, &ptp_caps);
		if (ret == 0) {
			caps = (struct precision_clock_caps){
				.flags = precision_clock_ptp_cap_flags(ptp_caps.flags),
				.resolution_ns = ptp_caps.resolution_ns,
				.max_phase_adjust_ns = ptp_caps.max_adjust_ns,
				.min_rate_ppb = ptp_caps.min_rate_ppb,
				.max_rate_ppb = ptp_caps.max_rate_ppb,
			};
		} else if (ret != -ENOTSUP) {
			return ret;
		}
	}

	adapter->ptp_clock = ptp_clock;
	adapter->caps = caps;
	adapter->clock = (struct precision_clock){
		.api = &precision_clock_ptp_api,
		.adapter = adapter,
		.domain = domain,
	};

	return 0;
}

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_ptp_clock

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include "nsi_errno.h"
#include "ptp_clock_native_bottom.h"

static int ptp_clock_set_native(const struct device *clk, struct net_ptp_time *tm)
{
	ARG_UNUSED(clk);
	ARG_UNUSED(tm);

	/* We cannot set the host device time so this function
	 * does nothing.
	 */

	return 0;
}

static int ptp_clock_get_native(const struct device *clk, struct net_ptp_time *tm)
{
	int ret;

	ARG_UNUSED(clk);

	ret = ptp_clock_native_gettime(&tm->second, &tm->nanosecond);

	if (ret < 0) {
		return -nsi_errno_from_mid(-ret);
	}

	return 0;
}

static int ptp_clock_adjust_native(const struct device *clk, int increment)
{
	ARG_UNUSED(clk);
	ARG_UNUSED(increment);

	/* We cannot adjust the host device time so this function
	 * does nothing.
	 */

	return 0;
}

static int ptp_clock_rate_adjust_native(const struct device *clk, double ratio)
{
	ARG_UNUSED(clk);
	ARG_UNUSED(ratio);

	/* We cannot adjust the host device time so this function
	 * does nothing.
	 */

	return 0;
}

static int ptp_clock_get_caps_native(const struct device *clk, struct ptp_clock_caps *caps)
{
	ARG_UNUSED(clk);

	if (caps == NULL) {
		return -EINVAL;
	}

	*caps = (struct ptp_clock_caps){
		.flags = PTP_CLOCK_CAP_READ,
		.resolution_ns = 1,
		.max_adjust_ns = 0,
		.min_rate_ppb = 0,
		.max_rate_ppb = 0,
	};

	return 0;
}

static DEVICE_API(ptp_clock, api) = {
	.set = ptp_clock_set_native,
	.get = ptp_clock_get_native,
	.adjust = ptp_clock_adjust_native,
	.rate_adjust = ptp_clock_rate_adjust_native,
	.get_caps = ptp_clock_get_caps_native,
};

#define PTP_CLOCK_NATIVE_INIT(inst)                                                                \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, NULL, POST_KERNEL,                           \
			      CONFIG_PTP_CLOCK_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_NATIVE_INIT)

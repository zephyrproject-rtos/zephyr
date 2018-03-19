/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ptp_clock.h>

/* ptp_clock dedicated section limiters */
extern struct ptp_clock __ptp_clock_start[];
extern struct ptp_clock __ptp_clock_end[];

struct ptp_clock *ptp_clock_lookup_by_dev(struct device *dev)
{
	struct ptp_clock *clk;

	for (clk = __ptp_clock_start; clk != __ptp_clock_end; clk++) {
		if (clk->dev == dev) {
			return clk;
		}
	}

	return NULL;
}

/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/prometheus/summary.h>

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_summary, CONFIG_PROMETHEUS_LOG_LEVEL);

int prometheus_summary_observe(struct prometheus_summary *summary, double value)
{
	if (!summary) {
		return -EINVAL;
	}

	/* increment count */
	summary->count++;

	/* update sum */
	summary->sum += value;

	return 0;
}

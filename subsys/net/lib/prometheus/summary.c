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

int prometheus_summary_observe_set(struct prometheus_summary *summary,
				   double value, unsigned long count)
{
	unsigned long old_count;
	double old_sum;

	if (summary == NULL) {
		return -EINVAL;
	}

	if (value == summary->sum && count == summary->count) {
		return 0;
	}

	old_count = summary->count;
	if (count < old_count) {
		LOG_DBG("Cannot set summary count to a lower value");
		return -EINVAL;
	}

	summary->count += (count - old_count);

	old_sum = summary->sum;
	summary->sum += (value - old_sum);

	return 0;
}

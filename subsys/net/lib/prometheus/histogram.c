/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/prometheus/histogram.h>

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_histogram, CONFIG_PROMETHEUS_LOG_LEVEL);

int prometheus_histogram_observe(struct prometheus_histogram *histogram, double value)
{
	if (!histogram) {
		return -EINVAL;
	}

	/* increment count */
	histogram->count++;

	/* update sum */
	histogram->sum += value;

	/* find appropriate bucket */
	for (size_t i = 0; i < histogram->num_buckets; ++i) {
		if (value <= histogram->buckets[i].upper_bound) {
			/* increment count for the bucket */
			histogram->buckets[i].count++;

			LOG_DBG("value: %f, bucket: %f, count: %lu", value,
				histogram->buckets[i].upper_bound, histogram->buckets[i].count);

			break;
		}
	}

	return 0;
}

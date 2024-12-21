/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROMETHEUS_HISTOGRAM_H_
#define ZEPHYR_INCLUDE_PROMETHEUS_HISTOGRAM_H_

/**
 * @file
 *
 * @brief Prometheus histogram APIs.
 *
 * @addtogroup prometheus
 * @{
 */

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/prometheus/metric.h>

#include <stddef.h>

/**
 * @brief Prometheus histogram bucket definition.
 *
 * This structure defines a Prometheus histogram bucket.
 */
struct prometheus_histogram_bucket {
	/** Upper bound value of bucket */
	double upper_bound;
	/** Cumulative count of observations in the bucket */
	unsigned long count;
};

/**
 * @brief Type used to represent a Prometheus histogram metric.
 *
 * * References
 * * See https://prometheus.io/docs/concepts/metric_types/#histogram
 */
struct prometheus_histogram {
	/** Base of the Prometheus histogram metric */
	struct prometheus_metric *base;
	/** Array of buckets in the histogram */
	struct prometheus_histogram_bucket *buckets;
	/** Number of buckets in the histogram */
	size_t num_buckets;
	/** Sum of all observed values in the histogram */
	double sum;
	/** Total count of observations in the histogram */
	unsigned long count;
};

/**
 * @brief Prometheus Histogram definition.
 *
 * This macro defines a Histogram metric.
 *
 * @param _name The channel's name.
 * @param _detail The metric base.
 *
 * Example usage:
 * @code{.c}
 *
 * struct prometheus_metric http_request_gauge = {
 *	.type = PROMETHEUS_HISTOGRAM,
 *	.name = "http_request_histograms",
 *	.description = "HTTP request histogram",
 *	.num_labels = 1,
 *	.labels = {
 *		{ .key = "request_latency", .value = "request_latency_seconds",}
 *	},
 * };
 *
 * PROMETHEUS_HISTOGRAM_DEFINE(test_histogram, &test_histogram_metric);
 *
 * @endcode
 */
#define PROMETHEUS_HISTOGRAM_DEFINE(_name, _detail)                                                \
	static STRUCT_SECTION_ITERABLE(prometheus_histogram, _name) = {.base = (void *)(_detail),  \
								       .buckets = NULL,            \
								       .num_buckets = 0,           \
								       .sum = 0,                   \
								       .count = 0}

/**
 * @brief Observe a value in a Prometheus histogram metric
 *
 * Observes the specified value in the given histogram metric.
 *
 * @param histogram Pointer to the histogram metric to observe.
 * @param value Value to observe in the histogram metric.
 * @return 0 on success, -EINVAL if the value is negative.
 */
int prometheus_histogram_observe(struct prometheus_histogram *histogram, double value);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_HISTOGRAM_H_ */

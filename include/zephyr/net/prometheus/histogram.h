/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 * Copyright (c) 2024 Nordic Semiconductor
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
	struct prometheus_metric base;
	/** Array of buckets in the histogram */
	struct prometheus_histogram_bucket *buckets;
	/** Number of buckets in the histogram */
	size_t num_buckets;
	/** Sum of all observed values in the histogram */
	double sum;
	/** Total count of observations in the histogram */
	unsigned long count;
	/** User data */
	void *user_data;
};

/**
 * @brief Prometheus Histogram definition.
 *
 * This macro defines a Histogram metric. If you want to make the histogram static,
 * then add "static" keyword before the PROMETHEUS_HISTOGRAM_DEFINE.
 *
 * @param _name The histogram metric name.
 * @param _desc Histogram description
 * @param _label Label for the metric. Additional labels can be added at runtime.
 * @param _collector Collector to map this metric. Can be set to NULL if it not yet known.
 * @param ... Optional user data specific to this metric instance.
 *
 * Example usage:
 * @code{.c}
 *
 * PROMETHEUS_HISTOGRAM_DEFINE(http_request_histogram, "HTTP request histogram",
 *                         ({ .key = "request_latency", .value = "request_latency_seconds" }),
 *                         NULL);
 *
 * @endcode
 */
#define PROMETHEUS_HISTOGRAM_DEFINE(_name, _desc, _label, _collector, ...) \
	STRUCT_SECTION_ITERABLE(prometheus_histogram, _name) = {	\
		.base.name = STRINGIFY(_name),				\
		.base.type = PROMETHEUS_HISTOGRAM,			\
		.base.description = _desc,				\
		.base.labels[0] = __DEBRACKET _label,			\
		.base.num_labels = 1,					\
		.base.collector = _collector,				\
		.buckets = NULL,					\
		.num_buckets = 0,					\
		.sum = 0.0,						\
		.count = 0U,						\
		.user_data = COND_CODE_0(				\
			NUM_VA_ARGS_LESS_1(LIST_DROP_EMPTY(__VA_ARGS__, _)), \
			(NULL),						\
			(GET_ARG_N(1, __VA_ARGS__))),			\
	}

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

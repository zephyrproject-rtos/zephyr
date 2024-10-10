/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROMETHEUS_COUNTER_H_
#define ZEPHYR_INCLUDE_PROMETHEUS_COUNTER_H_

/**
 * @file
 *
 * @brief Prometheus counter APIs.
 *
 * @addtogroup prometheus
 * @{
 */

#include <stdint.h>

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/prometheus/metric.h>

/**
 * @brief Type used to represent a Prometheus counter metric.
 *
 * * References
 * * See https://prometheus.io/docs/concepts/metric_types/#counter
 */
struct prometheus_counter {
	/** Base of the Prometheus counter metric */
	struct prometheus_metric *base;
	/** Value of the Prometheus counter metric */
	uint64_t value;
};

/**
 * @brief Prometheus Counter definition.
 *
 * This macro defines a Counter metric.
 *
 * @param _name The channel's name.
 * @param _detail The metric base.
 *
 * Example usage:
 * @code{.c}
 *
 * struct prometheus_metric http_request_counter = {
 *	.type = PROMETHEUS_COUNTER,
 *	.name = "http_request_counter",
 *	.description = "HTTP request counter",
 *	.num_labels = 1,
 *	.labels = {
 *		{ .key = "http_request", .value = "request_count",}
 *	},
 *};
 *
 * PROMETHEUS_COUNTER_DEFINE(test_counter, &test_counter_metric);
 * @endcode
 */
#define PROMETHEUS_COUNTER_DEFINE(_name, _detail)                                                  \
	static STRUCT_SECTION_ITERABLE(prometheus_counter, _name) = {.base = (void *)(_detail),    \
								     .value = 0}

/**
 * @brief Increment the value of a Prometheus counter metric
 * Increments the value of the specified counter metric by one.
 * @param counter Pointer to the counter metric to increment.
 * @return 0 on success, negative errno on error.
 */
int prometheus_counter_inc(struct prometheus_counter *counter);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_COUNTER_H_ */

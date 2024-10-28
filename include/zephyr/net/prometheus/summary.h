/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROMETHEUS_SUMMARY_H_
#define ZEPHYR_INCLUDE_PROMETHEUS_SUMMARY_H_

/**
 * @file
 *
 * @brief Prometheus summary APIs.
 *
 * @addtogroup prometheus
 * @{
 */

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/prometheus/metric.h>

#include <stddef.h>

/**
 * @brief Prometheus summary quantile definition.
 *
 * This structure defines a Prometheus summary quantile.
 */
struct prometheus_summary_quantile {
	/** Quantile of the summary */
	double quantile;
	/** Value of the quantile */
	double value;
};

/**
 * @brief Type used to represent a Prometheus summary metric.
 *
 * * References
 * * See https://prometheus.io/docs/concepts/metric_types/#summary
 */
struct prometheus_summary {
	/** Base of the Prometheus summary metric */
	struct prometheus_metric *base;
	/** Array of quantiles associated with the Prometheus summary metric */
	struct prometheus_summary_quantile *quantiles;
	/** Number of quantiles associated with the Prometheus summary metric */
	size_t num_quantiles;
	/** Sum of all observed values in the summary metric */
	double sum;
	/** Total count of observations in the summary metric */
	unsigned long count;
};

/**
 * @brief Prometheus Summary definition.
 *
 * This macro defines a Summary metric.
 *
 * @param _name The channel's name.
 * @param _detail The metric base.
 *
 * Example usage:
 * @code{.c}
 *
 * struct prometheus_metric http_request_gauge = {
 *	.type = PROMETHEUS_SUMMARY,
 *	.name = "http_request_summary",
 *	.description = "HTTP request summary",
 *	.num_labels = 1,
 *	.labels = {
 *		{ .key = "request_latency", .value = "request_latency_seconds",}
 *	},
 * };
 *
 * PROMETHEUS_SUMMARY_DEFINE(test_summary, &test_summary_metric);
 *
 * @endcode
 */

#define PROMETHEUS_SUMMARY_DEFINE(_name, _detail)                                                  \
	static STRUCT_SECTION_ITERABLE(prometheus_summary, _name) = {.base = (void *)(_detail),    \
								     .quantiles = NULL,            \
								     .num_quantiles = 0,           \
								     .sum = 0,                     \
								     .count = 0}

/**
 * @brief Observes a value in a Prometheus summary metric
 *
 * Observes the specified value in the given summary metric.
 *
 * @param summary Pointer to the summary metric to observe.
 * @param value Value to observe in the summary metric.
 * @return 0 on success, -EINVAL if the value is negative.
 */
int prometheus_summary_observe(struct prometheus_summary *summary, double value);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_SUMMARY_H_ */

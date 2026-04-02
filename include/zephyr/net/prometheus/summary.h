/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 * Copyright (c) 2024 Nordic Semiconductor
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
	/** User data */
	void *user_data;
};

/**
 * @brief Type used to represent a Prometheus summary metric.
 *
 * * References
 * * See https://prometheus.io/docs/concepts/metric_types/#summary
 */
struct prometheus_summary {
	/** Base of the Prometheus summary metric */
	struct prometheus_metric base;
	/** Array of quantiles associated with the Prometheus summary metric */
	struct prometheus_summary_quantile *quantiles;
	/** Number of quantiles associated with the Prometheus summary metric */
	size_t num_quantiles;
	/** Sum of all observed values in the summary metric */
	double sum;
	/** Total count of observations in the summary metric */
	unsigned long count;
	/** User data */
	void *user_data;
};

/**
 * @brief Prometheus Summary definition.
 *
 * This macro defines a Summary metric. If you want to make the summary static,
 * then add "static" keyword before the PROMETHEUS_SUMMARY_DEFINE.
 *
 * @param _name The summary metric name.
 * @param _desc Summary description
 * @param _label Label for the metric. Additional labels can be added at runtime.
 * @param _collector Collector to map this metric. Can be set to NULL if it not yet known.
 * @param ... Optional user data specific to this metric instance.
 *
 *
 * Example usage:
 * @code{.c}
 *
 * PROMETHEUS_SUMMARY_DEFINE(http_request_summary, "HTTP request summary",
 *                           ({ .key = "request_latency",
 *                              .value = "request_latency_seconds" }), NULL);
 *
 * @endcode
 */

#define PROMETHEUS_SUMMARY_DEFINE(_name, _desc, _label, _collector, ...) \
	STRUCT_SECTION_ITERABLE(prometheus_summary, _name) = {		\
		.base.name = STRINGIFY(_name),				\
		.base.type = PROMETHEUS_SUMMARY,			\
		.base.description = _desc,				\
		.base.labels[0] = __DEBRACKET _label,			\
		.base.num_labels = 1,					\
		.base.collector = _collector,				\
		.quantiles = NULL,					\
		.num_quantiles = 0,					\
		.sum = 0.0,						\
		.count = 0U,						\
		.user_data = COND_CODE_0(				\
			NUM_VA_ARGS_LESS_1(LIST_DROP_EMPTY(__VA_ARGS__, _)), \
			(NULL),						\
			(GET_ARG_N(1, __VA_ARGS__))),			\
	}

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
 * @brief Set the summary value to specific value.
 * The new value must be higher than the current value. This function can be used
 * if we cannot add individual increments to the summary but need to periodically
 * update the counter value. This function will add the difference between the
 * new value and the old value to the summary fields.
 * @param summary Pointer to the summary metric to increment.
 * @param value New value of the summary.
 * @param count New counter value of the summary.
 * @return 0 on success, negative errno on error.
 */
int prometheus_summary_observe_set(struct prometheus_summary *summary,
				   double value, unsigned long count);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_SUMMARY_H_ */

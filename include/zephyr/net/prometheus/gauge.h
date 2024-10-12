/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROMETHEUS_GAUGE_H_
#define ZEPHYR_INCLUDE_PROMETHEUS_GAUGE_H_

/**
 * @file
 *
 * @brief Prometheus gauge APIs.
 *
 * @addtogroup prometheus
 * @{
 */

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/prometheus/metric.h>

/**
 * @brief Type used to represent a Prometheus gauge metric.
 *
 * * References
 * * See https://prometheus.io/docs/concepts/metric_types/#gauge
 */
struct prometheus_gauge {
	/** Base of the Prometheus gauge metric */
	struct prometheus_metric *base;
	/** Value of the Prometheus gauge metric */
	double value;
};

/**
 * @brief Prometheus Gauge definition.
 *
 * This macro defines a Gauge metric.
 *
 * @param _name The channel's name.
 * @param _detail The metric base.
 *
 * Example usage:
 * @code{.c}
 *
 * struct prometheus_metric http_request_gauge = {
 *	.type = PROMETHEUS_GAUGE,
 *	.name = "http_request_gauge",
 *	.description = "HTTP request gauge",
 *	.num_labels = 1,
 *	.labels = {
 *		{ .key = "http_request", .value = "request_count",}
 *	},
 * };
 *
 * PROMETHEUS_GAUGE_DEFINE(test_gauge, &test_gauge_metric);
 *
 * @endcode
 */
#define PROMETHEUS_GAUGE_DEFINE(_name, _detail)                                                    \
	static STRUCT_SECTION_ITERABLE(prometheus_gauge, _name) = {.base = (void *)(_detail),      \
								   .value = 0}

/**
 * @brief Set the value of a Prometheus gauge metric
 *
 * Sets the value of the specified gauge metric to the given value.
 *
 * @param gauge Pointer to the gauge metric to set.
 * @param value Value to set the gauge metric to.
 *
 * @return 0 on success, -EINVAL if the value is negative.
 */
int prometheus_gauge_set(struct prometheus_gauge *gauge, double value);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_GAUGE_H_ */

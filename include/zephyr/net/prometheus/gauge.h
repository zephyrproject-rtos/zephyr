/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 * Copyright (c) 2024 Nordic Semiconductor
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
	struct prometheus_metric base;
	/** Value of the Prometheus gauge metric */
	double value;
	/** User data */
	void *user_data;
};

/**
 * @brief Prometheus Gauge definition.
 *
 * This macro defines a Gauge metric. If you want to make the gauge static,
 * then add "static" keyword before the PROMETHEUS_GAUGE_DEFINE.
 *
 * @param _name The gauge metric name.
 * @param _desc Gauge description
 * @param _label Label for the metric. Additional labels can be added at runtime.
 * @param _collector Collector to map this metric. Can be set to NULL if it not yet known.
 * @param ... Optional user data specific to this metric instance.
 *
 * Example usage:
 * @code{.c}
 *
 * PROMETHEUS_GAUGE_DEFINE(http_request_gauge, "HTTP request gauge",
 *                         ({ .key = "http_request", .value = "request_count" }),
 *                         NULL);
 *
 * @endcode
 */
#define PROMETHEUS_GAUGE_DEFINE(_name, _desc, _label, _collector, ...)	\
	STRUCT_SECTION_ITERABLE(prometheus_gauge, _name) = {		\
		.base.name = STRINGIFY(_name),				\
		.base.type = PROMETHEUS_GAUGE,				\
		.base.description = _desc,				\
		.base.labels[0] = __DEBRACKET _label,			\
		.base.num_labels = 1,					\
		.base.collector = _collector,				\
		.value = 0.0,						\
		.user_data = COND_CODE_0(				\
			NUM_VA_ARGS_LESS_1(LIST_DROP_EMPTY(__VA_ARGS__, _)), \
			(NULL),						\
			(GET_ARG_N(1, __VA_ARGS__))),			\
	}

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

/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROMETHEUS_COLLECTOR_H_
#define ZEPHYR_INCLUDE_PROMETHEUS_COLLECTOR_H_

/**
 * @file
 *
 * @brief Prometheus collector APIs.
 *
 * @defgroup prometheus Prometheus API
 * @since 4.0
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/prometheus/metric.h>

#include <stddef.h>

/**
 * @brief Prometheus collector definition
 *
 * This structure defines a Prometheus collector.
 */
struct prometheus_collector {
	/** Name of the collector */
	const char *name;
	/** Array of metrics associated with the collector */
	struct prometheus_metric *metric[CONFIG_PROMETHEUS_MAX_METRICS];
	/** Number of metrics associated with the collector */
	size_t size;
};

/**
 * @brief Prometheus Collector definition.
 *
 * This macro defines a Collector.
 *
 * @param _name The collector's name.
 */
#define PROMETHEUS_COLLECTOR_DEFINE(_name)                                                         \
	static STRUCT_SECTION_ITERABLE(prometheus_collector, _name) = {                            \
		.name = STRINGIFY(_name), .size = 0, .metric = {0}}

/**
 * @brief Register a metric with a Prometheus collector
 *
 * Registers the specified metric with the given collector.
 *
 * @param collector Pointer to the collector to register the metric with.
 * @param metric Pointer to the metric to register.
 *
 * @return 0 if successful, otherwise a negative error code.
 * @retval -EINVAL Invalid arguments.
 * @retval -ENOMEM Not enough memory to register the metric.
 */
int prometheus_collector_register_metric(struct prometheus_collector *collector,
					 struct prometheus_metric *metric);

/**
 * @brief Get a metric from a Prometheus collector
 *
 * Retrieves the metric with the specified name from the given collector.
 *
 * @param collector Pointer to the collector to retrieve the metric from.
 * @param name Name of the metric to retrieve.
 * @return Pointer to the retrieved metric, or NULL if not found.
 */
const void *prometheus_collector_get_metric(const struct prometheus_collector *collector,
					    const char *name);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_COLLECTOR_H_ */

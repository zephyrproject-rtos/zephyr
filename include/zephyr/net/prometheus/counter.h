/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 * Copyright (c) 2024 Nordic Semiconductor
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
	struct prometheus_metric base;
	/** Value of the Prometheus counter metric */
	uint64_t value;
	/** User data */
	void *user_data;
};

/**
 * @brief Prometheus Counter definition.
 *
 * This macro defines a Counter metric. If you want to make the counter static,
 * then add "static" keyword before the PROMETHEUS_COUNTER_DEFINE.
 *
 * @param _name The counter metric name
 * @param _desc Counter description
 * @param _label Label for the metric. Additional labels can be added at runtime.
 * @param _collector Collector to map this metric. Can be set to NULL if it not yet known.
 * @param ... Optional user data specific to this metric instance.
 *
 * Example usage:
 * @code{.c}
 *
 * PROMETHEUS_COUNTER_DEFINE(http_request_counter, "HTTP request counter",
 *                           ({ .key = "http_request", .value = "request_count" }),
 *                           NULL);
 * @endcode
 */
#define PROMETHEUS_COUNTER_DEFINE(_name, _desc, _label, _collector, ...) \
	STRUCT_SECTION_ITERABLE(prometheus_counter, _name) = {		\
		.base.name = STRINGIFY(_name),				\
		.base.type = PROMETHEUS_COUNTER,			\
		.base.description = _desc,				\
		.base.labels[0] = __DEBRACKET _label,			\
		.base.num_labels = 1,					\
		.base.collector = _collector,				\
		.value = 0ULL,						\
		.user_data = COND_CODE_0(				\
			NUM_VA_ARGS_LESS_1(LIST_DROP_EMPTY(__VA_ARGS__, _)), \
			(NULL),						\
			(GET_ARG_N(1, __VA_ARGS__))),			\
	}

/**
 * @brief Increment the value of a Prometheus counter metric
 * Increments the value of the specified counter metric by arbitrary amount.
 * @param counter Pointer to the counter metric to increment.
 * @param value Amount to increment the counter by.
 * @return 0 on success, negative errno on error.
 */
int prometheus_counter_add(struct prometheus_counter *counter, uint64_t value);

/**
 * @brief Increment the value of a Prometheus counter metric
 * Increments the value of the specified counter metric by one.
 * @param counter Pointer to the counter metric to increment.
 * @return 0 on success, negative errno on error.
 */
static inline int prometheus_counter_inc(struct prometheus_counter *counter)
{
	return prometheus_counter_add(counter, 1ULL);
}

/**
 * @brief Set the counter value to specific value.
 * The new value must be higher than the current value. This function can be used
 * if we cannot add individual increments to the counter but need to periodically
 * update the counter value. This function will add the difference between the
 * new value and the old value to the counter.
 * @param counter Pointer to the counter metric to increment.
 * @param value New value of the counter.
 * @return 0 on success, negative errno on error.
 */
int prometheus_counter_set(struct prometheus_counter *counter, uint64_t value);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_COUNTER_H_ */

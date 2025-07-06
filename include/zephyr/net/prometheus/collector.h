/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 * Copyright (c) 2024 Nordic Semiconductor
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

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/prometheus/metric.h>

#include <stddef.h>

struct prometheus_collector;

/**
 * @typedef prometheus_scrape_cb_t
 * @brief Callback used to scrape a collector for a specific metric.
 *
 * @param collector A valid pointer on the collector to scrape
 * @param metric A valid pointer on the metric to scrape
 * @param user_data A valid pointer to a user data or NULL
 *
 * @return 0 if successful, otherwise a negative error code.
 */
typedef int (*prometheus_scrape_cb_t)(struct prometheus_collector *collector,
				      struct prometheus_metric *metric,
				      void *user_data);

/**
 * @brief Prometheus collector definition
 *
 * This structure defines a Prometheus collector.
 */
struct prometheus_collector {
	/** Name of the collector */
	const char *name;
	/** Array of metrics associated with the collector */
	sys_slist_t metrics;
	/** Mutex to protect the metrics list manipulation */
	struct k_mutex lock;
	/** User callback function. If set, then the metric data is fetched
	 * via the function callback.
	 */
	prometheus_scrape_cb_t user_cb;
	/** User data */
	void *user_data;
};

/**
 * @brief Prometheus Collector definition.
 *
 * This macro defines a Collector.
 *
 * @param _name The collector's name.
 * @param ... Optional user callback function. If set, this function is called
 *            when the collector is scraped. The function should be of type
 *            prometheus_scrape_cb_t.
 *            Optional user data to pass to the user callback function.
 */
#define PROMETHEUS_COLLECTOR_DEFINE(_name, ...)	\
	STRUCT_SECTION_ITERABLE(prometheus_collector, _name) = {	\
		.name = STRINGIFY(_name),				\
		.metrics = SYS_SLIST_STATIC_INIT(&_name.metrics),	\
		.lock = Z_MUTEX_INITIALIZER(_name.lock),		\
		.user_cb = COND_CODE_0(					\
			NUM_VA_ARGS_LESS_1(				\
				LIST_DROP_EMPTY(__VA_ARGS__, _)),	\
			(NULL),						\
			(GET_ARG_N(1, __VA_ARGS__))),			\
		.user_data = COND_CODE_0(				\
			NUM_VA_ARGS_LESS_1(__VA_ARGS__), (NULL),	\
			(GET_ARG_N(1,					\
				   GET_ARGS_LESS_N(1, __VA_ARGS__)))),	\
	}

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
const void *prometheus_collector_get_metric(struct prometheus_collector *collector,
					    const char *name);

/** @cond INTERNAL_HIDDEN */

enum prometheus_walk_state {
	PROMETHEUS_WALK_START,
	PROMETHEUS_WALK_CONTINUE,
	PROMETHEUS_WALK_STOP,
};

struct prometheus_collector_walk_context {
	struct prometheus_collector *collector;
	struct prometheus_metric *metric;
	struct prometheus_metric *tmp;
	enum prometheus_walk_state state;
};

/** @endcond */

/**
 * @brief Walk through all metrics in a Prometheus collector and format them
 *        into a buffer.
 *
 * @param ctx Pointer to the walker context.
 * @param buffer Pointer to the buffer to store the formatted metrics.
 * @param buffer_size Size of the buffer.
 * @return 0 if successful and we went through all metrics, -EAGAIN if we
 *	 need to call this function again, any other negative error code
 *	 means an error occurred.
 */
int prometheus_collector_walk_metrics(struct prometheus_collector_walk_context *ctx,
				      uint8_t *buffer, size_t buffer_size);

/**
 * @brief Initialize the walker context to walk through all metrics.
 *
 * @param ctx Pointer to the walker context.
 * @param collector Pointer to the collector to walk through.
 *
 * @return 0 if successful, otherwise a negative error code.
 */
static inline int prometheus_collector_walk_init(struct prometheus_collector_walk_context *ctx,
						 struct prometheus_collector *collector)
{
	if (collector == NULL) {
		return -EINVAL;
	}

	ctx->collector = collector;
	ctx->state = PROMETHEUS_WALK_START;
	ctx->metric = NULL;
	ctx->tmp = NULL;

	return 0;
}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_COLLECTOR_H_ */

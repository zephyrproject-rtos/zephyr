/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROMETHEUS_METRIC_H_
#define ZEPHYR_INCLUDE_PROMETHEUS_METRIC_H_

/**
 * @file
 *
 * @brief Prometheus metric interface.
 *
 * @addtogroup prometheus
 * @{
 */

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/slist.h>
#include <zephyr/net/prometheus/label.h>

/**
 * @brief Prometheus metric types.
 *
 * * References
 * * See https://prometheus.io/docs/concepts/metric_types
 */
enum prometheus_metric_type {
	/** Prometheus Counter */
	PROMETHEUS_COUNTER = 0,
	/** Prometheus Gauge */
	PROMETHEUS_GAUGE,
	/** Prometheus Summary */
	PROMETHEUS_SUMMARY,
	/** Prometheus Histogram */
	PROMETHEUS_HISTOGRAM,
};

/**
 * @brief Type used to represent a Prometheus metric base.
 *
 * Every metric has a prometheus_metric structure associated used
 * to control the metric access and usage.
 */
struct prometheus_metric {
	/** Slist metric list node */
	sys_snode_t node;
	/** Back pointer to the collector that this metric belongs to */
	struct prometheus_collector *collector;
	/** Back pointer to the actual metric (counter, gauge, etc.).
	 * This is just a temporary solution, ultimate goal is to place
	 * this generic metrict struct into the actual metric struct.
	 */
	void *metric;
	/** Type of the Prometheus metric. */
	enum prometheus_metric_type type;
	/** Name of the Prometheus metric. */
	const char *name;
	/** Description of the Prometheus metric. */
	const char *description;
	/** Labels associated with the Prometheus metric. */
	struct prometheus_label labels[MAX_PROMETHEUS_LABELS_PER_METRIC];
	/** Number of labels associated with the Prometheus metric. */
	int num_labels;
	/** User defined data */
	void *user_data;
	/* Add any other necessary fields */
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_METRIC_H_ */

/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
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

#define MAX_METRIC_NAME_LENGTH        32
#define MAX_METRIC_DESCRIPTION_LENGTH 64

/**
 * @brief Type used to represent a Prometheus metric base.
 *
 * Every metric has a prometheus_metric structure associated used
 * to control the metric access and usage.
 */
struct prometheus_metric {
	/** Type of the Prometheus metric. */
	enum prometheus_metric_type type;
	/** Name of the Prometheus metric. */
	char name[MAX_METRIC_NAME_LENGTH];
	/** Description of the Prometheus metric. */
	char description[MAX_METRIC_DESCRIPTION_LENGTH];
	/** Labels associated with the Prometheus metric. */
	struct prometheus_label labels[MAX_PROMETHEUS_LABELS_PER_METRIC];
	/** Number of labels associated with the Prometheus metric. */
	int num_labels;
	/* Add any other necessary fields */
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_METRIC_H_ */

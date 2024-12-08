/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROMETHEUS_FORMATTER_H_
#define ZEPHYR_INCLUDE_PROMETHEUS_FORMATTER_H_

/**
 * @file
 *
 * @brief Prometheus formatter APIs.
 *
 * @addtogroup prometheus
 * @{
 */

#include <zephyr/net/prometheus/collector.h>

/**
 * @brief Format exposition data for Prometheus
 *
 * Formats the exposition data collected by the specified collector into the provided buffer.
 * Function will format metric data according to Prometheus text-based format
 *
 * @param collector Pointer to the collector containing the data to format.
 * @param buffer Pointer to the buffer where the formatted exposition data will be stored.
 * @param buffer_size Size of the buffer.
 *
 * @return 0 on success, negative errno on error.
 */
int prometheus_format_exposition(struct prometheus_collector *collector, char *buffer,
				 size_t buffer_size);

/**
 * @brief Format exposition data for one metric for Prometheus
 *
 * Formats the exposition data of one specific metric into the provided buffer.
 * Function will format metric data according to Prometheus text-based format.
 *
 * @param metric Pointer to the metric containing the data to format.
 * @param buffer Pointer to the buffer where the formatted exposition data will be stored.
 * @param buffer_size Size of the buffer.
 * @param written How many bytes have been written to the buffer.
 *
 * @return 0 on success, negative errno on error.
 */
int prometheus_format_one_metric(struct prometheus_metric *metric, char *buffer,
				 size_t buffer_size, int *written);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_FORMATTER_H_ */

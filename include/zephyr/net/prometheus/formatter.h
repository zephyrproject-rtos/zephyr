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
 * Function to format metric data according to Prometheus text-based format
 *
 * @param collector Pointer to the collector containing the data to format.
 * @param buffer Pointer to the buffer where the formatted exposition data will be stored.
 * @param buffer_size Size of the buffer.
 *
 * @return 0 on success, negative errno on error.
 */
int prometheus_format_exposition(const struct prometheus_collector *collector, char *buffer,
				 size_t buffer_size);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_FORMATTER_H_ */

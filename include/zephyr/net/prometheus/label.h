/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROMETHEUS_LABEL_H_
#define ZEPHYR_INCLUDE_PROMETHEUS_LABEL_H_

/**
 * @file
 *
 * @brief Prometheus label interface.
 *
 * @addtogroup prometheus
 * @{
 */

/* maximum length of label key */
#define MAX_PROMETHEUS_LABEL_KEY_LENGTH   16
/* maximum length of label value */
#define MAX_PROMETHEUS_LABEL_VALUE_LENGTH 16
/* maximum namber of labels per metric */
#define MAX_PROMETHEUS_LABELS_PER_METRIC  5

/**
 * @brief Prometheus label definition.
 *
 * This structure defines a Prometheus label.
 */
struct prometheus_label {
	/** Prometheus metric label key */
	char key[MAX_PROMETHEUS_LABEL_KEY_LENGTH];
	/** Prometheus metric label value */
	char value[MAX_PROMETHEUS_LABEL_VALUE_LENGTH];
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PROMETHEUS_LABEL_H_ */

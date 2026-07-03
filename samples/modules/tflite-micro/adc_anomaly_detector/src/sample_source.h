/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "features.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int sample_source_init(void);
int sample_source_load_window(size_t sample_window, int16_t *window, size_t len);
enum adc_anomaly_label sample_source_expected_label(size_t sample_window);
size_t sample_source_window_start(size_t sample_window);
const char *sample_source_label_name(enum adc_anomaly_label label);
const char *sample_source_name(void);
bool sample_source_has_expected_labels(void);

#ifdef __cplusplus
}
#endif

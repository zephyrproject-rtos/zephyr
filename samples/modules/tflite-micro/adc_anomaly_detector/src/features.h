/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_ANOMALY_WINDOW_SIZE 64U
#define ADC_ANOMALY_HOP_SIZE 32U
#define ADC_ANOMALY_NUM_FEATURES 6U
#define ADC_ANOMALY_NUM_CLASSES 3U
#define ADC_ANOMALY_WINDOWS_PER_CLASS 2U
#define ADC_ANOMALY_TOTAL_WINDOWS \
	(ADC_ANOMALY_NUM_CLASSES * ADC_ANOMALY_WINDOWS_PER_CLASS)

enum adc_anomaly_label {
	ADC_ANOMALY_NORMAL = 0,
	ADC_ANOMALY_DRIFT = 1,
	ADC_ANOMALY_TRANSIENT = 2,
};

struct adc_anomaly_features {
	float mean;
	float rms;
	float peak;
	float zero_crossing_rate;
	float slope;
	float energy;
};

void adc_anomaly_extract_features(const int16_t *samples, size_t len,
				  struct adc_anomaly_features *features);

#ifdef __cplusplus
}
#endif

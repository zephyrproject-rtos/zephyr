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

#define ADC_ANOMALY_RING_BUFFER_SIZE 64U

struct adc_anomaly_ring_buffer {
	int16_t data[ADC_ANOMALY_RING_BUFFER_SIZE];
	size_t head;
	size_t count;
};

void adc_anomaly_ring_buffer_reset(struct adc_anomaly_ring_buffer *ring);
void adc_anomaly_ring_buffer_push(struct adc_anomaly_ring_buffer *ring,
				  int16_t sample);
void adc_anomaly_ring_buffer_push_many(struct adc_anomaly_ring_buffer *ring,
				       const int16_t *samples, size_t len);
void adc_anomaly_ring_buffer_copy_window(const struct adc_anomaly_ring_buffer *ring,
					 int16_t *window, size_t len);

#ifdef __cplusplus
}
#endif

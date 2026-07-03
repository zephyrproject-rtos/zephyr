/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ring_buffer.h"

void adc_anomaly_ring_buffer_reset(struct adc_anomaly_ring_buffer *ring)
{
	ring->head = 0U;
	ring->count = 0U;
}

void adc_anomaly_ring_buffer_push(struct adc_anomaly_ring_buffer *ring,
				  int16_t sample)
{
	ring->data[ring->head] = sample;
	ring->head = (ring->head + 1U) % ADC_ANOMALY_RING_BUFFER_SIZE;

	if (ring->count < ADC_ANOMALY_RING_BUFFER_SIZE) {
		ring->count++;
	}
}

void adc_anomaly_ring_buffer_push_many(struct adc_anomaly_ring_buffer *ring,
				       const int16_t *samples, size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		adc_anomaly_ring_buffer_push(ring, samples[i]);
	}
}

void adc_anomaly_ring_buffer_copy_window(const struct adc_anomaly_ring_buffer *ring,
					 int16_t *window, size_t len)
{
	size_t start = (ring->head + ADC_ANOMALY_RING_BUFFER_SIZE - len) %
			       ADC_ANOMALY_RING_BUFFER_SIZE;

	for (size_t i = 0U; i < len; i++) {
		window[i] = ring->data[(start + i) % ADC_ANOMALY_RING_BUFFER_SIZE];
	}
}

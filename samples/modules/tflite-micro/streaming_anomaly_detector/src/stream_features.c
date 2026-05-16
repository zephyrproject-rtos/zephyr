/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stream_features.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool stream_crossed_zero(int16_t previous, int16_t current)
{
	return (previous < 0 && current >= 0) ||
		       (previous >= 0 && current < 0);
}

void stream_extract_features(const int16_t *samples, size_t len,
			     struct stream_features *features)
{
	float sum;
	float sum_sq;
	float peak;
	uint32_t zero_crossings;

	sum = 0.0f;
	sum_sq = 0.0f;
	peak = 0.0f;
	zero_crossings = 0U;

	for (size_t i = 0U; i < len; i++) {
		float value = (float)samples[i];
		float abs_value = fabsf(value);

		sum += value;
		sum_sq += value * value;

		if (abs_value > peak) {
			peak = abs_value;
		}

		if (i > 0U && stream_crossed_zero(samples[i - 1U], samples[i])) {
			zero_crossings++;
		}
	}

	features->mean = sum / (float)len;
	features->rms = sqrtf(sum_sq / (float)len);
	features->peak = peak;
	features->zero_crossing_rate = (float)zero_crossings / (float)(len - 1U);
	features->slope = (float)(samples[len - 1U] - samples[0]) / (float)len;
	features->energy = sum_sq;
}

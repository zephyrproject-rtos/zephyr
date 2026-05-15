/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "features.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool crossed_zero(int16_t previous, int16_t current)
{
	return (previous < 0 && current >= 0) ||
		       (previous >= 0 && current < 0);
}

void time_series_extract_features(const int16_t *samples, size_t len,
				  struct time_series_features *features)
{
	float sum = 0.0f;
	float sum_sq = 0.0f;
	float peak = 0.0f;
	uint32_t zero_crossings;

	zero_crossings = 0U;

	for (size_t i = 0; i < len; i++) {
		float value = (float)samples[i];
		float abs_value = fabsf(value);

		sum += value;
		sum_sq += value * value;

		if (abs_value > peak) {
			peak = abs_value;
		}

		if (i > 0U && crossed_zero(samples[i - 1U], samples[i])) {
			zero_crossings++;
		}
	}

	features->mean = sum / (float)len;
	features->rms = sqrtf(sum_sq / (float)len);
	features->peak = peak;
	features->zero_crossing_rate = (float)zero_crossings / (float)(len - 1U);
	features->energy = sum_sq;
}

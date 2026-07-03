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

struct time_series_features {
	float mean;
	float rms;
	float peak;
	float zero_crossing_rate;
	float energy;
};

void time_series_extract_features(const int16_t *samples,
				  size_t len,
				  struct time_series_features *features);

#ifdef __cplusplus
}
#endif

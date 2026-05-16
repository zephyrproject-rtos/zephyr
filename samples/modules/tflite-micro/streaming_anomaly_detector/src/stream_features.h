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

#define STREAM_WINDOW_SIZE 64U
#define STREAM_HOP_SIZE 32U
#define STREAM_NUM_FEATURES 6U

struct stream_features {
	float mean;
	float rms;
	float peak;
	float zero_crossing_rate;
	float slope;
	float energy;
};

void stream_extract_features(const int16_t *samples, size_t len,
			     struct stream_features *features);

#ifdef __cplusplus
}
#endif

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

#define FFT_EVENT_WINDOW_SIZE 64U
#define FFT_EVENT_NUM_CLASSES 3U
#define FFT_EVENT_NUM_FEATURES 8U

struct fft_event_features {
	float mean;
	float rms;
	float peak;
	float zero_crossing_rate;
	float low_band_energy;
	float mid_band_energy;
	float high_band_energy;
	float spectral_centroid;
};

int fft_event_extract_features(const int16_t *samples, size_t len,
			       struct fft_event_features *features);

#ifdef __cplusplus
}
#endif

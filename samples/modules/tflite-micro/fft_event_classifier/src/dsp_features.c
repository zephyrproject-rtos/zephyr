/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dsp_features.h"

#include <arm_math.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FFT_EVENT_NUM_BINS (FFT_EVENT_WINDOW_SIZE / 2U)
#define FFT_EVENT_PI 3.14159265358979323846f

static arm_rfft_fast_instance_f32 fft_instance;
static bool fft_initialized;
static float32_t fft_input[FFT_EVENT_WINDOW_SIZE];
static float32_t fft_output[FFT_EVENT_WINDOW_SIZE];

static bool fft_event_crossed_zero(int16_t previous, int16_t current)
{
	return (previous < 0 && current >= 0) ||
		       (previous >= 0 && current < 0);
}

static float fft_event_band_energy(const float *magnitudes, size_t first,
				   size_t last)
{
	float energy;

	energy = 0.0f;

	for (size_t i = first; i <= last; i++) {
		energy += magnitudes[i] * magnitudes[i];
	}

	return energy;
}

static int fft_event_init_fft(void)
{
	if (fft_initialized) {
		return 0;
	}

	if (arm_rfft_fast_init_f32(&fft_instance,
				   FFT_EVENT_WINDOW_SIZE) != ARM_MATH_SUCCESS) {
		return -EIO;
	}

	fft_initialized = true;

	return 0;
}

static void fft_event_extract_time_features(const int16_t *samples, size_t len,
					    struct fft_event_features *features)
{
	float sum;
	float sum_sq;
	float peak;
	uint32_t zero_crossings;

	sum = 0.0f;
	sum_sq = 0.0f;
	peak = 0.0f;
	zero_crossings = 0U;

	for (size_t i = 0; i < len; i++) {
		float value = (float)samples[i];
		float abs_value = fabsf(value);

		sum += value;
		sum_sq += value * value;

		if (abs_value > peak) {
			peak = abs_value;
		}

		if (i > 0U && fft_event_crossed_zero(samples[i - 1U], samples[i])) {
			zero_crossings++;
		}
	}

	features->mean = sum / (float)len;
	features->rms = sqrtf(sum_sq / (float)len);
	features->peak = peak;
	features->zero_crossing_rate = (float)zero_crossings / (float)(len - 1U);
}

static void fft_event_apply_window(const int16_t *samples,
				   const struct fft_event_features *features,
				   size_t len)
{
	for (size_t i = 0; i < len; i++) {
		float centered = (float)samples[i] - features->mean;
		float phase = (2.0f * FFT_EVENT_PI * (float)i) /
			      (float)(FFT_EVENT_WINDOW_SIZE - 1U);
		float window = 0.5f - (0.5f * cosf(phase));

		fft_input[i] = centered * window;
	}
}

static void fft_event_compute_magnitudes(float *magnitudes)
{
	magnitudes[0] = fabsf(fft_output[0]);
	magnitudes[FFT_EVENT_NUM_BINS] = fabsf(fft_output[1]);

	for (size_t i = 1U; i < FFT_EVENT_NUM_BINS; i++) {
		float real = fft_output[2U * i];
		float imag = fft_output[(2U * i) + 1U];

		magnitudes[i] = sqrtf((real * real) + (imag * imag));
	}
}

static void fft_event_extract_spectral_features(float *magnitudes,
					struct fft_event_features *features)
{
	float low_energy;
	float mid_energy;
	float high_energy;
	float total_energy;
	float weighted_sum;
	float magnitude_sum;

	low_energy = fft_event_band_energy(magnitudes, 1U, 4U);
	mid_energy = fft_event_band_energy(magnitudes, 5U, 12U);
	high_energy = fft_event_band_energy(magnitudes, 13U, FFT_EVENT_NUM_BINS);
	total_energy = low_energy + mid_energy + high_energy;

	if (total_energy <= 0.0f) {
		total_energy = 1.0f;
	}

	features->low_band_energy = low_energy / total_energy;
	features->mid_band_energy = mid_energy / total_energy;
	features->high_band_energy = high_energy / total_energy;

	weighted_sum = 0.0f;
	magnitude_sum = 0.0f;

	for (size_t i = 1U; i <= FFT_EVENT_NUM_BINS; i++) {
		weighted_sum += (float)i * magnitudes[i];
		magnitude_sum += magnitudes[i];
	}

	if (magnitude_sum <= 0.0f) {
		features->spectral_centroid = 0.0f;
	} else {
		features->spectral_centroid =
			(weighted_sum / magnitude_sum) / (float)FFT_EVENT_NUM_BINS;
	}
}

int fft_event_extract_features(const int16_t *samples, size_t len,
			       struct fft_event_features *features)
{
	float magnitudes[FFT_EVENT_NUM_BINS + 1U];
	int ret;

	if (len != FFT_EVENT_WINDOW_SIZE || samples == NULL || features == NULL) {
		return -EINVAL;
	}

	ret = fft_event_init_fft();
	if (ret != 0) {
		return ret;
	}

	fft_event_extract_time_features(samples, len, features);
	fft_event_apply_window(samples, features, len);
	arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);
	fft_event_compute_magnitudes(magnitudes);
	fft_event_extract_spectral_features(magnitudes, features);

	return 0;
}

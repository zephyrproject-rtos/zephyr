/*
 * Copyright (c) 2026 Mahad Faisal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dsp_features.h"
#include "input_data.h"

#include <stddef.h>
#include <stdint.h>

static const int16_t normal_window[FFT_EVENT_WINDOW_SIZE] = {
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195,
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195
};

static const int16_t high_frequency_window[FFT_EVENT_WINDOW_SIZE] = {
	0, 831, 924, 195, -707, -981, -383, 556,
	1000, 556, -383, -981, -707, 195, 924, 831,
	0, -831, -924, -195, 707, 981, 383, -556,
	-1000, -556, 383, 981, 707, -195, -924, -831,
	0, 831, 924, 195, -707, -981, -383, 556,
	1000, 556, -383, -981, -707, 195, 924, 831,
	0, -831, -924, -195, 707, 981, 383, -556,
	-1000, -556, 383, 981, 707, -195, -924, -831
};

static const int16_t transient_window[FFT_EVENT_WINDOW_SIZE] = {
	0, 176, 344, 500, 636, 748, 831, 883,
	900, 883, 831, 748, 636, 500, 344, 176,
	2500, -176, -344, -500, -636, -748, -831, -883,
	-900, -883, -831, -748, -636, -500, -344, -176,
	0, 176, 344, 500, 636, 748, 831, 883,
	900, 883, 831, 748, 636, 500, 344, 176,
	-2300, -176, -344, -500, -636, -748, -831, -883,
	-900, -883, -831, -748, -636, -500, -344, -176
};

static const struct fft_event_window windows[FFT_EVENT_NUM_CLASSES] = {
	{
		.name = "normal",
		.label = FFT_EVENT_NORMAL,
		.samples = normal_window,
		.len = FFT_EVENT_WINDOW_SIZE,
	},
	{
		.name = "high_frequency",
		.label = FFT_EVENT_HIGH_FREQUENCY,
		.samples = high_frequency_window,
		.len = FFT_EVENT_WINDOW_SIZE,
	},
	{
		.name = "transient",
		.label = FFT_EVENT_TRANSIENT,
		.samples = transient_window,
		.len = FFT_EVENT_WINDOW_SIZE,
	}
};

const struct fft_event_window *fft_event_get_window(size_t index)
{
return &windows[index % FFT_EVENT_NUM_CLASSES];
}

const char *fft_event_label_name(enum fft_event_label label)
{
switch (label) {
case FFT_EVENT_NORMAL:
return "normal";
case FFT_EVENT_HIGH_FREQUENCY:
return "high_frequency";
case FFT_EVENT_TRANSIENT:
return "transient";
default:
return "unknown";
}
}

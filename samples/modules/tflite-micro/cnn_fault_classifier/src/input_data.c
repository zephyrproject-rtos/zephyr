/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "input_data.h"

#include <stddef.h>
#include <stdint.h>

static const int16_t normal_window[CNN_FAULT_WINDOW_SIZE] = {
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195,
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195
};

static const int16_t high_frequency_fault_window[CNN_FAULT_WINDOW_SIZE] = {
	0, 696, 883, 424, -344, -861, -748, -88,
	636, 896, 500, -261, -831, -794, -176, 571,
	900, 571, -176, -794, -831, -261, 500, 896,
	636, -88, -748, -861, -344, 424, 883, 696,
	0, -696, -883, -424, 344, 861, 748, 88,
	-636, -896, -500, 261, 831, 794, 176, -571,
	-900, -571, 176, 794, 831, 261, -500, -896,
	-636, 88, 748, 861, 344, -424, -883, -696
};

static const int16_t impulse_fault_window[CNN_FAULT_WINDOW_SIZE] = {
	0, 156, 306, 444, 566, 665, 739, 785,
	800, 785, 739, 665, 566, 444, 306, 156,
	2600, -156, -306, -444, -566, -665, -739, -785,
	-800, -785, -739, -665, -566, -444, -306, -156,
	0, 156, 306, 444, 566, 665, 739, 785,
	800, 785, 739, 665, 566, 444, 306, -2244,
	0, -156, -306, -444, -566, -665, -739, -785,
	-800, -785, -739, -665, -566, -444, -306, -156
};

static const struct cnn_fault_window windows[CNN_FAULT_NUM_CLASSES] = {
	{
		.name = "normal",
		.label = CNN_FAULT_NORMAL,
		.samples = normal_window,
		.len = CNN_FAULT_WINDOW_SIZE,
	},
	{
		.name = "high_frequency_fault",
		.label = CNN_FAULT_HIGH_FREQUENCY,
		.samples = high_frequency_fault_window,
		.len = CNN_FAULT_WINDOW_SIZE,
	},
	{
		.name = "impulse_fault",
		.label = CNN_FAULT_IMPULSE,
		.samples = impulse_fault_window,
		.len = CNN_FAULT_WINDOW_SIZE,
	}
};

const struct cnn_fault_window *cnn_fault_get_window(size_t index)
{
return &windows[index % CNN_FAULT_NUM_CLASSES];
}

const char *cnn_fault_label_name(enum cnn_fault_label label)
{
switch (label) {
case CNN_FAULT_NORMAL:
return "normal";
case CNN_FAULT_HIGH_FREQUENCY:
return "high_frequency_fault";
case CNN_FAULT_IMPULSE:
return "impulse_fault";
default:
return "unknown";
}
}

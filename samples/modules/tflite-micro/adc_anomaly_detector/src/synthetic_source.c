/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sample_source.h"

#include "ring_buffer.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#define ADC_ANOMALY_SAMPLES_PER_CLASS (ADC_ANOMALY_WINDOW_SIZE + ADC_ANOMALY_HOP_SIZE)

static const int16_t normal_stream[ADC_ANOMALY_SAMPLES_PER_CLASS] = {
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195,
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195,
	0, 195, 383, 556, 707, 831, 924, 981,
	1000, 981, 924, 831, 707, 556, 383, 195,
	0, -195, -383, -556, -707, -831, -924, -981,
	-1000, -981, -924, -831, -707, -556, -383, -195
};

static const int16_t drift_stream[ADC_ANOMALY_SAMPLES_PER_CLASS] = {
	0, 76, 152, 227, 299, 370, 437, 501,
	561, 616, 667, 712, 752, 786, 814, 836,
	852, 862, 865, 862, 853, 839, 818, 793,
	763, 728, 690, 647, 602, 555, 506, 455,
	404, 353, 303, 253, 206, 161, 119, 80,
	46, 15, -10, -30, -45, -54, -56, -53,
	-44, -28, -6, 22, 56, 96, 142, 192,
	248, 308, 372, 439, 509, 582, 656, 732,
	808, 885, 960, 1035, 1108, 1178, 1245, 1309,
	1369, 1425, 1475, 1521, 1561, 1595, 1623, 1645,
	1661, 1670, 1673, 1670, 1662, 1647, 1627, 1601,
	1571, 1537, 1498, 1456, 1411, 1363, 1314, 1264
};

static const int16_t transient_stream[ADC_ANOMALY_SAMPLES_PER_CLASS] = {
	0, 156, 306, 444, 566, 665, 739, 785,
	800, 785, 739, 665, 566, 444, 306, 156,
	0, -156, -306, -444, 1934, -665, -739, -785,
	-800, -785, -739, -665, -566, -444, -306, -156,
	0, 156, 306, 444, 566, 665, 739, 785,
	800, 785, 739, 665, 566, 444, 306, 156,
	0, -156, -306, -444, -566, -665, -739, -785,
	-800, -785, -739, -665, -566, -444, -306, -156,
	0, 156, 306, 444, 566, 665, 739, 785,
	-1500, 785, 739, 665, 566, 444, 306, 156,
	0, -156, -306, -444, -566, -665, -739, -785,
	-800, -785, -739, -665, -566, -444, -306, -156
};

static const int16_t *const streams[ADC_ANOMALY_NUM_CLASSES] = {
	[0] = normal_stream,
	[1] = drift_stream,
	[2] = transient_stream
};

static struct adc_anomaly_ring_buffer sample_ring;

int sample_source_init(void)
{
adc_anomaly_ring_buffer_reset(&sample_ring);

return 0;
}

int sample_source_load_window(size_t sample_window, int16_t *window, size_t len)
{
enum adc_anomaly_label label;
const int16_t *samples;
size_t position;

if (len != ADC_ANOMALY_WINDOW_SIZE) {
return -EINVAL;
}

label = sample_source_expected_label(sample_window);
samples = streams[(size_t)label];
position = sample_window % ADC_ANOMALY_WINDOWS_PER_CLASS;

if (position == 0U) {
adc_anomaly_ring_buffer_reset(&sample_ring);
adc_anomaly_ring_buffer_push_many(&sample_ring, samples,
  ADC_ANOMALY_WINDOW_SIZE);
} else {
adc_anomaly_ring_buffer_push_many(&sample_ring,
  &samples[ADC_ANOMALY_WINDOW_SIZE],
  ADC_ANOMALY_HOP_SIZE);
}

adc_anomaly_ring_buffer_copy_window(&sample_ring, window,
	ADC_ANOMALY_WINDOW_SIZE);

return 0;
}

enum adc_anomaly_label sample_source_expected_label(size_t sample_window)
{
return (enum adc_anomaly_label)(sample_window / ADC_ANOMALY_WINDOWS_PER_CLASS);
}

size_t sample_source_window_start(size_t sample_window)
{
return (sample_window % ADC_ANOMALY_WINDOWS_PER_CLASS) * ADC_ANOMALY_HOP_SIZE;
}

const char *sample_source_label_name(enum adc_anomaly_label label)
{
switch (label) {
case ADC_ANOMALY_NORMAL:
return "normal";
case ADC_ANOMALY_DRIFT:
return "drift";
case ADC_ANOMALY_TRANSIENT:
return "transient";
default:
return "unknown";
}
}

const char *sample_source_name(void)
{
return "synthetic";
}

bool sample_source_has_expected_labels(void)
{
return true;
}

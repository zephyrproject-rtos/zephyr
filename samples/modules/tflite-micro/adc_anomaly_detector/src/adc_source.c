/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sample_source.h"

#include "ring_buffer.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

#if !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "ADC source requires zephyr,user io-channels in devicetree"
#endif

static const struct adc_dt_spec adc_channel =
	ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
static struct adc_anomaly_ring_buffer sample_ring;

static int read_adc_sample(int16_t *sample)
{
	int16_t raw;
	int32_t centered;
	int32_t midpoint;
	struct adc_sequence sequence;
	int ret;

	sequence = (struct adc_sequence) {
		.buffer = &raw,
		.buffer_size = sizeof(raw),
	};

	ret = adc_sequence_init_dt(&adc_channel, &sequence);
	if (ret < 0) {
		return ret;
	}

	ret = adc_read_dt(&adc_channel, &sequence);
	if (ret < 0) {
		return ret;
	}

	midpoint = 1 << (adc_channel.resolution - 1);
	centered = (int32_t)raw - midpoint;
	*sample = (int16_t)(centered * 2);

	return 0;
}

static int push_adc_samples(size_t count)
{
	for (size_t i = 0U; i < count; i++) {
		int16_t sample;
		int ret;

		ret = read_adc_sample(&sample);
		if (ret < 0) {
			return ret;
		}

		adc_anomaly_ring_buffer_push(&sample_ring, sample);
		k_sleep(K_USEC(CONFIG_TFLM_ADC_ANOMALY_DETECTOR_ADC_INTERVAL_US));
	}

	return 0;
}

int sample_source_init(void)
{
	int ret;

	if (!adc_is_ready_dt(&adc_channel)) {
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&adc_channel);
	if (ret < 0) {
		return ret;
	}

	adc_anomaly_ring_buffer_reset(&sample_ring);

	return 0;
}

int sample_source_load_window(size_t sample_window, int16_t *window, size_t len)
{
	size_t position;
	int ret;

	if (len != ADC_ANOMALY_WINDOW_SIZE) {
		return -EINVAL;
	}

	position = sample_window % ADC_ANOMALY_WINDOWS_PER_CLASS;

	if (position == 0U) {
		adc_anomaly_ring_buffer_reset(&sample_ring);
		ret = push_adc_samples(ADC_ANOMALY_WINDOW_SIZE);
	} else {
		ret = push_adc_samples(ADC_ANOMALY_HOP_SIZE);
	}

	if (ret < 0) {
		return ret;
	}

	adc_anomaly_ring_buffer_copy_window(&sample_ring, window,
					    ADC_ANOMALY_WINDOW_SIZE);

	return 0;
}

enum adc_anomaly_label sample_source_expected_label(size_t sample_window)
{
	(void)sample_window;

	return ADC_ANOMALY_NORMAL;
}

size_t sample_source_window_start(size_t sample_window)
{
	return (sample_window % ADC_ANOMALY_WINDOWS_PER_CLASS) *
		       ADC_ANOMALY_HOP_SIZE;
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
	return "adc";
}

bool sample_source_has_expected_labels(void)
{
	return false;
}

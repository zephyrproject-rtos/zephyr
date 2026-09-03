/*
 * Copyright (c) 2026 Shontal Biton
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, 4);

#define SAMPLING_RATE 8000
#define NUM_SAMPLES 16000

enum adc_action sample_end_event(const struct device *dev, const struct adc_sequence *sequence,
				 uint16_t sampling_index)
{
	LOG_DBG("Finished sampling for 2 seconds");
	LOG_HEXDUMP_INF(sequence->buffer, 20, "Buffer values are: ");

	return ADC_ACTION_CONTINUE;
}

uint16_t samples_buffer[NUM_SAMPLES] = {0};

int main(void)
{
	const struct device *timer = DEVICE_DT_GET(DT_CHOSEN(adc_counter));
	const struct adc_dt_spec channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
	const struct counter_top_cfg timer_cfg = {
		.ticks = (counter_get_frequency(timer) / SAMPLING_RATE),
	};
	const struct adc_sequence_options options = {
		.callback = sample_end_event,
		.extra_samplings = NUM_SAMPLES - 1,
	};
	struct adc_sequence seq = {
		.buffer = samples_buffer,
		.buffer_size = sizeof(samples_buffer),
		.options = &options,
	};
	int err;

	err = adc_sequence_init_dt(&channel, &seq);
	if (err) {
		LOG_ERR("Failed to init sequence");
		return err;
	}

	err = counter_start(timer);
	if (err) {
		LOG_ERR("Failed to start timer with error code: %d", err);
		return err;
	}

	err = counter_set_top_value(timer, &timer_cfg);
	if (err) {
		LOG_ERR("Failed to set timer configuration");
		return err;
	}

	err = adc_read_async_dt(&channel, &seq, NULL);
	if (err) {
		LOG_ERR("Failed to start reading");
		return err;
	}

	return 0;
}

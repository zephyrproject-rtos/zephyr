/*
 * Copyright (c) 2024 Centro de Inovacao EDGE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

/* ADC node from the devicetree. */
#define ADC_NODE DT_ALIAS(adc0)

/* Data of ADC device specified in devicetree. */
static const struct device *adc = DEVICE_DT_GET(ADC_NODE);

/* Data array of ADC channels for the specified ADC. */
static const struct adc_channel_cfg channel_cfgs[] = {
	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};

/* Get the number of channels defined on the DTS. */
#define CHANNEL_COUNT ARRAY_SIZE(channel_cfgs)

int main(void)
{
	int err;
	uint32_t count = 0;
	uint16_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];

	/* Options for the sequence sampling. */
	const struct adc_sequence_options options = {
		.extra_samplings = CONFIG_SEQUENCE_SAMPLES - 1,
		.interval_us = 0,
	};

	/* Configure the sampling sequence to be made. */
	struct adc_sequence sequence = {
		.buffer = channel_reading,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(channel_reading),
		.resolution = CONFIG_SEQUENCE_RESOLUTION,
		.options = &options,
	};

	if (!device_is_ready(adc)) {
		printf("ADC controller device %s not ready\n", adc->name);
		return 0;
	}

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
		sequence.channels |= BIT(channel_cfgs[i].channel_id);
		err = adc_channel_setup(adc, &channel_cfgs[i]);
		if (err < 0) {
			printf("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}

	while (1) {
		printf("ADC sequence reading [%u]:\n", count++);
		k_msleep(1000);

		err = adc_read(adc, &sequence);
		if (err < 0) {
			printf("Could not read (%d)\n", err);
			continue;
		}

		for (size_t channel_index = 0U; channel_index < CHANNEL_COUNT; channel_index++) {
			int32_t val_mv;

			printf("- %s, channel %" PRId32 ", %" PRId32 " sequence samples:\n",
			       adc->name, channel_cfgs[channel_index].channel_id,
			       CONFIG_SEQUENCE_SAMPLES);
			for (size_t sample_index = 0U; sample_index < CONFIG_SEQUENCE_SAMPLES;
			     sample_index++) {

				val_mv = channel_reading[sample_index][channel_index];

				printf("- - %" PRId32, val_mv);
				err = adc_raw_to_millivolts(channel_cfgs[channel_index].reference,
							    channel_cfgs[channel_index].gain,
							    CONFIG_SEQUENCE_RESOLUTION, &val_mv);

				/* conversion to mV may not be supported, skip if not */
				if ((err < 0) || channel_cfgs[channel_index].reference == 0) {
					printf(" (value in mV not available)\n");
				} else {
					printf(" = %" PRId32 "mV\n", val_mv);
				}
			}
		}
	}

	return 0;
}

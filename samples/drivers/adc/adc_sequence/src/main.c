/*
 * Copyright (c) 2024 Centro de Inovacao EDGE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

#define CHANNEL_NODE_SPEC_AND_COMMA(node_id) ADC_DT_SPEC_FROM_CHANNEL_NODE(node_id),

/*
 * Every channel node of an enabled ADC controller; the sequence reads those
 * on the controller of the first one.
 */
static const struct adc_dt_spec adc_channels[] = {
	ADC_DT_FOREACH_CHANNEL_NODE(CHANNEL_NODE_SPEC_AND_COMMA)
};

BUILD_ASSERT(ARRAY_SIZE(adc_channels) > 0, "No ADC channel described in devicetree");

/* Data of ADC device specified in devicetree. */
#define adc (adc_channels[0].dev)

/* Data array of ADC channel voltage references. */
static uint32_t vrefs_mv[ARRAY_SIZE(adc_channels)];

/* Upper bound of the channels in the sequence. */
#define CHANNEL_COUNT ARRAY_SIZE(adc_channels)

int main(void)
{
	int err;
	uint32_t count = 0;
#ifdef CONFIG_SEQUENCE_32BITS_REGISTERS
	uint32_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];
#else
	uint16_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];
#endif

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
		.oversampling = CONFIG_SEQUENCE_OVERSAMPLING,
		.options = &options,
	};

	if (!device_is_ready(adc)) {
		printf("ADC controller device %s not ready\n", adc->name);
		return 0;
	}

	/* Configure the channels of the first controller prior to sampling. */
	for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
		if (adc_channels[i].dev != adc) {
			continue;
		}
		sequence.channels |= BIT(adc_channels[i].channel_id);
		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printf("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
		vrefs_mv[i] = adc_channels[i].vref_mv;
		if ((vrefs_mv[i] == 0) &&
		    (adc_channels[i].channel_cfg.reference == ADC_REF_INTERNAL)) {
			vrefs_mv[i] = adc_ref_internal(adc);
		}
	}

#ifndef CONFIG_COVERAGE
	while (1) {
#else
	for (int k = 0; k < 10; k++) {
#endif
		printf("ADC sequence reading [%u]:\n", count++);
		k_msleep(1000);

		err = adc_read(adc, &sequence);
		if (err < 0) {
			printf("Could not read (%d)\n", err);
			continue;
		}

		for (size_t channel_index = 0U, sample_slot = 0U; channel_index < CHANNEL_COUNT;
		     channel_index++) {
			int32_t val_mv;

			if (adc_channels[channel_index].dev != adc) {
				continue;
			}
			printf("- %s, channel %" PRId32 ", %" PRId32 " sequence samples:\n",
			       adc->name, adc_channels[channel_index].channel_id,
			       CONFIG_SEQUENCE_SAMPLES);
			for (size_t sample_index = 0U; sample_index < CONFIG_SEQUENCE_SAMPLES;
			     sample_index++) {
				uint8_t res = CONFIG_SEQUENCE_RESOLUTION;

				/*
				 * If using differential mode, the 16/32 bit value
				 * in the ADC sample buffer should be a signed 2's
				 * complement value.
				 * Also reduce the resolution by 1 for the conversion
				 */
				if (adc_channels[channel_index].channel_cfg.differential) {
#ifdef CONFIG_SEQUENCE_32BITS_REGISTERS
					val_mv = (int32_t)
						channel_reading[sample_index][sample_slot];
#else
					val_mv = (int32_t)((int16_t)channel_reading[sample_index]
										   [channel_index]);
#endif
					res -= 1;
				} else {
					val_mv = channel_reading[sample_index][sample_slot];
				}
				printf("- - %" PRId32, val_mv);
				err = adc_raw_to_millivolts(
					vrefs_mv[channel_index],
					adc_channels[channel_index].channel_cfg.gain, res, &val_mv);

				/* conversion to mV may not be supported, skip if not */
				if ((err < 0) || vrefs_mv[channel_index] == 0) {
					printf(" (value in mV not available)\n");
				} else {
					printf(" = %" PRId32 "mV\n", val_mv);
				}
			}
			sample_slot++;
		}
	}

	return 0;
}

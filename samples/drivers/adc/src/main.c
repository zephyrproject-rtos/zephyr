/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/adc.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

void main(void)
{
	int err;
	int16_t sample_buffer[1];
	struct adc_sequence sequence = {
		.buffer      = sample_buffer,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(sample_buffer),
	};

	/* Configure channels individually prior to sampling. */
	for (uint8_t i = 0; i < ARRAY_SIZE(adc_channels); i++) {
		if (!device_is_ready(adc_channels[i].dev)) {
			printk("ADC device not found\n");
			return;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel (%d)\n", err);
			return;
		}
	}

	while (1) {
		printk("ADC reading:\n");
		for (uint8_t i = 0; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val_mv;

			printk("- %s, channel %d: ",
			       adc_channels[i].dev->name,
			       adc_channels[i].channel_id);

			(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

			err = adc_read(adc_channels[i].dev, &sequence);
			if (err < 0) {
				printk("error %d\n", err);
				continue;
			} else {
				printk("%d", sample_buffer[0]);
			}

			/* conversion to mV may not be supported, skip if not */
			val_mv = sample_buffer[0];
			err = adc_raw_to_millivolts_dt(&adc_channels[i],
						       &val_mv);
			if (err < 0) {
				printk(" (value in mV not available)\n");
			} else {
				printk(" = %d mV\n", val_mv);
			}
		}

		k_sleep(K_MSEC(1000));
	}
}

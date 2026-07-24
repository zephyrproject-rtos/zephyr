/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Smile
 */

#include <zephyr/drivers/adc.h>

/* Auxiliary macro to obtain channel vref, if available. */
#define CHANNEL_VREF(node_id) DT_PROP_OR(node_id, zephyr_vref_mv, 0)

/* ADC node from devicetree. */
#define ADC_NODE DT_ALIAS(adc0)

/* Get the number of channels defined in devicetree. */
#define CHANNEL_COUNT DT_CHILD_NUM(ADC_NODE)

/* Data of ADC device specified in devicetree. */
const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

/* Data array of ADC channels for the specified ADC. */
static const struct adc_channel_cfg channels_cfgs[] = {
	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))
};

/* Retrieve vref_mv from the first channel */
static int32_t vref_mv = GET_ARG_N(1, DT_FOREACH_CHILD_SEP(ADC_NODE, CHANNEL_VREF, (,)));

/* Callback function for threshold events */
static void callback(const struct device *dev, uint8_t channel_id, uint32_t value,
		      void *user_data)
{
	printk("Threshold crossed on channel %d with value %d\n", channel_id, value);
}

struct k_poll_signal adc_signal;

int main(void)
{
	enum adc_gain gain = channels_cfgs[0].gain;
	int err;
	uint32_t channel_reading[CHANNEL_COUNT];
	uint32_t selected_channels = 0;
	int32_t ref_mv;
	int32_t upper_threshold = CONFIG_THRESHOLD_UPPER_VALUE_MV; /* mV */

	/* Initializing signal for asytnchronous reads */
	k_poll_signal_init(&adc_signal);

	struct k_poll_event events[1] = {
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &adc_signal),
	};

	/* Check if device is ready */
	if (!device_is_ready(adc_dev)) {
		printk("device not ready.\n");
		return 0;
	}

	/* Configure channels */
	for (int i = 0; i < CHANNEL_COUNT; i++) {
		selected_channels |= BIT(channels_cfgs[i].channel_id);

		err = adc_channel_setup(adc_dev, &channels_cfgs[i]);
		if (err < 0) {
			printk("Failed to configure channel %d : %d\n", i, err);
			return 0;
		}
	}

	/* Get voltage reference for first used channel */
	if (channels_cfgs[0].reference == ADC_REF_INTERNAL) {
		ref_mv = adc_ref_internal(adc_dev);
	} else {
		ref_mv = vref_mv;
	}

	/* Compute upper_threshold raw value based on first used channel configuration */
	err = adc_millivolts_to_raw(ref_mv, gain, CONFIG_THRESHOLD_RESOLUTION, &upper_threshold);
	if (err < 0) {
		printk("Failed to convert upper_threshold from millivots to raw : %d\n", err);
		return 0;
	}

	/* Initialize commun sequence for all used channels */
	struct adc_sequence sequence = {
		.buffer = channel_reading,
		.buffer_size = sizeof(channel_reading),
		.resolution = CONFIG_THRESHOLD_RESOLUTION,
		.channels = selected_channels,
		.calibrate = false,
		.oversampling = 0,
		.options = NULL,
		.threshold_mode = ADC_THRESHOLD_MODE_UPPER,
		.lower_threshold = 0,
		.upper_threshold = upper_threshold,
	};

	/* Setting callback function for threshold events */
	err = adc_set_threshold_callback(adc_dev, callback, NULL);
	if (err < 0) {
		printk("Error setting threshold callback : %d\n", err);
		return 0;
	}

	/* Start asynchronous reads and repeats on complete after 1 second */
	printk("Starting asynchronous reads, waiting for threshold events\n");
	while (1) {
		adc_read_async(adc_dev, &sequence, &adc_signal);
		k_poll(events, 1, K_FOREVER);

		int signaled, result;

		k_poll_signal_check(&adc_signal, &signaled, &result);

		if (signaled && (result == 0)) {
			k_sleep(K_MSEC(1000));
		} else {
			printk("Error on asynchronous read : %d\n", result);
			return 0;
		}

		k_poll_signal_reset(&adc_signal);
		events[0].state = K_POLL_STATE_NOT_READY;
	}

	return 0;
}

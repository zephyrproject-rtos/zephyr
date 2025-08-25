/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main.h"
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/opamp.h>

static int32_t sample_buffer;
static const struct adc_dt_spec adc_channel =
	ADC_DT_SPEC_GET(DT_PHANDLE(DT_PATH(zephyr_user), opamp));
static const struct device *opamp_dev =
	DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), opamp));
static const int32_t adc_read_delay_ms =
	DT_PROP(DT_PATH(zephyr_user), adc_read_delay_ms);

int main(void)
{
	int ret;

	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		.buffer_size = sizeof(sample_buffer),
	};

	if (!adc_is_ready_dt(&adc_channel)) {
		printf("ADC device %s is not ready\n", adc_channel.dev->name);
		return 0;
	}

	if (!device_is_ready(opamp_dev)) {
		printf("OPAMP device %s is not ready\n", opamp_dev->name);
		return 0;
	}

	ret = adc_channel_setup_dt(&adc_channel);
	if (ret < 0) {
		printf("ADC channel could not setup with code (%d)\n", ret);
		return 0;
	}

	ret = adc_sequence_init_dt(&adc_channel, &sequence);
	if (ret < 0) {
		printf("ADC sequence initialize failed with code (%d)\n", ret);
		return 0;
	}

	ret = opamp_enable(opamp_dev);
	if (ret < 0) {
		printf("OPAMP enable failed with code (%d)\n", ret);
		return 0;
	}

	if (adc_read_delay_ms > 0) {
		k_msleep(adc_read_delay_ms);
	}

	ret = adc_read_dt(&adc_channel, &sequence);
	if (ret < 0) {
		printf("ADC read failed with code (%d)\n", ret);
		return 0;
	}

	ret = adc_raw_to_millivolts(adc_channel.vref_mv,
				adc_channel.channel_cfg.gain,
				adc_channel.resolution, &sample_buffer);
	if (ret < 0) {
		printf("Convert to millivolts failed with code (%d)\n", ret);
		return 0;
	}

	printf("OPAMP output is: %d(mv)\n", sample_buffer);

#if OPAMP_SUPPORT_PROGRAMMABLE_GAIN
	for (uint8_t index = 0U; index < ARRAY_SIZE(test_gain); index++) {
		ret = opamp_set_gain(opamp_dev, test_gain[index]);
		if (ret < 0) {
			printf("OPAMP gain set failed with code (%d)\n", ret);
			return 0;
		}

		if (adc_read_delay_ms > 0) {
			k_msleep(adc_read_delay_ms);
		}

		ret = adc_read_dt(&adc_channel, &sequence);
		if (ret < 0) {
			printf("ADC read failed with code (%d)\n", ret);
			return 0;
		}

		ret = adc_raw_to_millivolts(adc_channel.vref_mv,
					adc_channel.channel_cfg.gain,
					adc_channel.resolution, &sample_buffer);
		if (ret < 0) {
			printf("Convert to millivolts failed with code (%d)\n", ret);
			return 0;
		}

		printf("OPAMP output is: %d(mv)\n", sample_buffer);
	}
#endif

	return 0;
}

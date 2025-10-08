/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/opamp.h>

#define OPAMP_SUPPORT_PROGRAMMABLE_GAIN					\
	DT_NODE_HAS_PROP(DT_PHANDLE(DT_PATH(zephyr_user), opamp),	\
			programmable_gain)

#if OPAMP_SUPPORT_PROGRAMMABLE_GAIN
static const enum opamp_gain gain[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_PHANDLE(DT_PATH(zephyr_user), opamp),
				programmable_gain, DT_ENUM_IDX_BY_IDX, (,))
};
uint8_t size = ARRAY_SIZE(gain);
#else
uint8_t size;
#endif

const char* opamp_gain2str[] = {
	[OPAMP_GAIN_1_7]  = "1/7",
	[OPAMP_GAIN_1_3]  = "1/3",
	[OPAMP_GAIN_1]    = "1",
	[OPAMP_GAIN_5_3]  = "5/3",
	[OPAMP_GAIN_2]    = "2",
	[OPAMP_GAIN_11_5] = "11/5",
	[OPAMP_GAIN_3]    = "3",
	[OPAMP_GAIN_4]    = "4",
	[OPAMP_GAIN_13_3] = "13/3",
	[OPAMP_GAIN_7]    = "7",
	[OPAMP_GAIN_8]    = "8",
	[OPAMP_GAIN_15]   = "15",
	[OPAMP_GAIN_16]   = "16",
	[OPAMP_GAIN_31]   = "31",
	[OPAMP_GAIN_32]   = "32",
	[OPAMP_GAIN_33]   = "33",
	[OPAMP_GAIN_63]   = "63",
	[OPAMP_GAIN_64]   = "64",
	[OPAMP_GAIN_64 + 1] = "default",
};

static int device_initialize(const struct adc_dt_spec *adc_channel,
			const struct device *opamp_dev,
			struct adc_sequence *seq)
{
	int ret;

	if (!adc_is_ready_dt(adc_channel)) {
		printf("ADC device %s is not ready\n", adc_channel->dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(opamp_dev)) {
		printf("OPAMP device %s is not ready\n", opamp_dev->name);
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(adc_channel);
	if (ret < 0) {
		printf("ADC channel could not setup with code (%d)\n", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(adc_channel, seq);
	if (ret < 0) {
		printf("ADC sequence initialize failed with code (%d)\n", ret);
		return ret;
	}

	return 0;
}

static int read_opamp_output(const struct adc_dt_spec *adc_channel,
			struct adc_sequence *seq,
			int32_t *sample_buffer)
{
	int ret;
	int32_t adc_read_delay_ms = DT_PROP(DT_PATH(zephyr_user), adc_read_delay_ms);

	if (adc_read_delay_ms > 0) {
		k_msleep(adc_read_delay_ms);
	}

	ret = adc_read_dt(adc_channel, seq);
	if (ret < 0) {
		printf("ADC read failed with code (%d)\n", ret);
		return ret;
	}

	ret = adc_raw_to_millivolts(adc_channel->vref_mv,
				adc_channel->channel_cfg.gain,
				adc_channel->resolution,
				sample_buffer);

	if (ret < 0) {
		printf("Convert to millivolts failed with code (%d)\n", ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	static const struct adc_dt_spec adc_channel =
		ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
	static const struct device *opamp_dev =
		DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), opamp));
	static int32_t sample_buffer;
	int ret;

	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		.buffer_size = sizeof(sample_buffer),
	};

	ret = device_initialize(&adc_channel, opamp_dev, &sequence);
	if (ret < 0) {
		printf("Initialization failed with code (%d)\n", ret);
		return 0;
	}


	uint16_t gain_value = 0;
	do {
#if OPAMP_SUPPORT_PROGRAMMABLE_GAIN
		/* For OPAMPs which support programmable gain, retrieve the gain array. */
		gain_value = gain[size - 1];
		ret = opamp_set_gain(opamp_dev, (enum opamp_gain)gain_value);
		if (ret < 0) {
			printf("OPAMP gain set failed with code (%d)\n", ret);
			return 0;
		}

		size--;
#else
		gain_value = OPAMP_GAIN_64 + 1; /* Default gain */
#endif
		ret = read_opamp_output(&adc_channel, &sequence, &sample_buffer);
		if (ret < 0) {
			printf("OPAMP output read failed with code (%d)\n", ret);
			return 0;
		}

		printf("OPAMP output (@gain: %s) is: %d(mv)\n",
			opamp_gain2str[gain_value], sample_buffer);
	} while (size);

	return 0;
}

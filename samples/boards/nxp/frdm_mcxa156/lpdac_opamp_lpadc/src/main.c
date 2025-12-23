/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/sys/printk.h>

#define CHANNEL_VREF(node_id) DT_PROP_OR(node_id, zephyr_vref_mv, 0)

static const struct adc_channel_cfg channel_cfgs[] = {
	DT_FOREACH_CHILD_SEP(DT_NODELABEL(lpadc0), ADC_CHANNEL_CFG_DT, (,))};

static uint32_t vrefs_mv[] = {DT_FOREACH_CHILD_SEP(DT_NODELABEL(lpadc0), CHANNEL_VREF, (,))};

#define CHANNEL_COUNT		ARRAY_SIZE(channel_cfgs)
#define ROUNDS			5
#define DAC_CHANNEL_ID		0
#define DAC_RESOLUTION		12
#define DAC_VALUE		372 /* 300mv output, 3300mv reference. */

/* Buffer for ROUNDS x CHANNEL_COUNT results */
static uint16_t readings[ROUNDS][CHANNEL_COUNT];

static int dac_setup(const struct device *dac)
{
	int ret;

	if (!device_is_ready(dac)) {
		printk("DAC device %s is not ready\n", dac->name);
		return -ENODEV;
	}

	const struct dac_channel_cfg dac_ch_cfg = {
		.channel_id  = DAC_CHANNEL_ID,
		.resolution  = DAC_RESOLUTION,
		.buffered = false,
	};

	ret = dac_channel_setup(dac, &dac_ch_cfg);

	if (ret != 0) {
		printk("Setting up of DAC channel failed with code %d\n", ret);
		return ret;
	}

	return 0;
}

static int adc_setup(const struct device *adc, struct adc_sequence *seq)
{
	int ret;

	if (!device_is_ready(adc)) {
		printk("ADC device not ready\n");
		return -ENODEV;
	}

	for (uint8_t i = 0U; i < CHANNEL_COUNT; i++) {
		ret = adc_channel_setup(adc, &channel_cfgs[i]);

		if (ret < 0) {
			printk("Could not setup channel #%u with code (%d)\n",
									(unsigned int)i, ret);
			return ret;
		}

		if ((vrefs_mv[i] == 0) && (channel_cfgs[i].reference == ADC_REF_INTERNAL)) {
			vrefs_mv[i] = adc_ref_internal(adc);
		}
	}

	static const struct adc_sequence_options opts = {
		.interval_us = 0,
		.extra_samplings = ROUNDS - 1,
	};

	seq->resolution = 16;
	seq->oversampling = 0;
	seq->buffer = readings;
	seq->buffer_size = sizeof(readings);
	seq->options = &opts;
	seq->channels = 0;

	for (uint8_t i = 0U; i < CHANNEL_COUNT; i++) {
		seq->channels |= BIT(channel_cfgs[i].channel_id);
	}

	return 0;
}

int main(void)
{
	const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(lpadc0));
	const struct device *dac = DEVICE_DT_GET(DT_NODELABEL(dac0));
	struct adc_sequence seq;
	int ret;

	ret = adc_setup(adc, &seq);
	if (ret) {
		printk("ADC setup failed with code %d\n", ret);
		return ret;
	}

	ret = dac_setup(dac);
	if (ret) {
		printk("DAC setup failed with code %d\n", ret);
		return ret;
	}

	ret = dac_write_value(dac, DAC_CHANNEL_ID, DAC_VALUE);
	if (ret != 0) {
		printk("Writing to DAC channel failed with code %d\n", ret);
		return ret;
	}

	ret = adc_read(adc, &seq);
	if (ret) {
		printk("ADC read failed with code %d\n", ret);
		return ret;
	}

	for (uint8_t r = 0U; r < ROUNDS; r++) {
		uint16_t output_raw = readings[r][0];
		uint16_t input_raw  = readings[r][1];
		int32_t out_mv = (int32_t)output_raw;
		int32_t in_mv  = (int32_t)input_raw;

		ret = adc_raw_to_millivolts(vrefs_mv[0], channel_cfgs[0].gain,
				seq.resolution, &out_mv);
		if (ret < 0) {
			printf("Convert to millivolts failed with code (%d)\n", ret);
			return ret;
		}

		ret = adc_raw_to_millivolts(vrefs_mv[1], channel_cfgs[1].gain,
				seq.resolution, &in_mv);
		if (ret < 0) {
			printf("Convert to millivolts failed with code (%d)\n", ret);
			return ret;
		}

		printk("round %u: output=%u (~%u mV), input=%u (~%u mV)\n",
			r, output_raw, out_mv, input_raw, in_mv);
	}

	while (1) {
		k_sleep(K_SECONDS(1));
	}
}

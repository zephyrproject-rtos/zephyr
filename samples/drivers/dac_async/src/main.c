/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/dac.h>
#include <drivers/hwtrig.h>

#if defined(CONFIG_BOARD_NUCLEO_L4R5ZI)
#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac1))
#define HWTRIG_DEVICE_NAME	DT_LABEL(DT_NODELABEL(hwtrig2))
#define DAC_CHANNEL_ID		1
#define DAC_RESOLUTION		12
#else
#error "Unsupported board."
#endif

static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id  = DAC_CHANNEL_ID,
	.resolution  = DAC_RESOLUTION,
	.enable_hw_trigger = true
};

static uint16_t dac_values[1 << DAC_RESOLUTION];
static uint16_t curr_values_pos = 0;

static void fill_dac_buffer(const struct device *dev_dac, uint8_t channel)
{
	int ret;
	while (true)
	{
		size_t size = (ARRAY_SIZE(dac_values) - curr_values_pos) * 2;
		ret = dac_fill_buffer(dev_dac, channel,
				(uint8_t *)&dac_values[curr_values_pos], size);

		curr_values_pos =
				(curr_values_pos + (ret / 2)) % ARRAY_SIZE(dac_values);
		if (ret < size) {
			return;
		}
	}
}

void dac_callback(const struct device *dev_dac, uint8_t channel, void *arg)
{
	if (channel == 1) {
		fill_dac_buffer(dev_dac, channel);
	}
}

void main(void)
{
	const struct device *dac_dev = device_get_binding(DAC_DEVICE_NAME);
	const struct device *hwtrig_dev = device_get_binding(HWTRIG_DEVICE_NAME);

	if (!dac_dev) {
		printk("Cannot get DAC device\n");
		return;
	}

	if (!hwtrig_dev) {
		printk("Cannot get HWTRIG device\n");
		return;
	}

	for (uint16_t i = 0; i < ARRAY_SIZE(dac_values); ++i) {
		dac_values[i] = i;
	}

	int ret = dac_channel_setup(dac_dev, &dac_ch_cfg);

	if (ret != 0) {
		printk("Setting up of DAC channel failed with code %d\n", ret);
		return;
	}

	dac_callback_set(dac_dev, dac_callback, NULL);

	dac_start_continious(dac_dev, DAC_CHANNEL_ID);
	fill_dac_buffer(dac_dev, DAC_CHANNEL_ID);

	printk("Generating sawtooth signal at DAC channel %d.\n",
		DAC_CHANNEL_ID);

	uint32_t freq = 1000; // 1 [KHz]
	hwtrig_enable(hwtrig_dev, &freq);
}

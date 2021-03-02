/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/dac.h>

#if defined(CONFIG_BOARD_NUCLEO_F091RC) || \
	defined(CONFIG_BOARD_NUCLEO_G071RB) || \
	defined(CONFIG_BOARD_NUCLEO_G431RB) || \
	defined(CONFIG_BOARD_NUCLEO_L073RZ) || \
	defined(CONFIG_BOARD_NUCLEO_L152RE)
#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac1))
#define DAC_CHANNEL_ID		1
#define DAC_RESOLUTION		12
#elif defined(CONFIG_BOARD_TWR_KE18F)
#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac0))
#define DAC_CHANNEL_ID		0
#define DAC_RESOLUTION		12
#elif defined(CONFIG_BOARD_FRDM_K64F)
#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac0))
#define DAC_CHANNEL_ID		0
#define DAC_RESOLUTION		12
#elif defined(CONFIG_BOARD_FRDM_K22F)
#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac0))
#define DAC_CHANNEL_ID		0
#define DAC_RESOLUTION		12
#elif defined(CONFIG_BOARD_ARDUINO_ZERO)
#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac0))
#define DAC_CHANNEL_ID		0
#define DAC_RESOLUTION		10
#else
#error "Unsupported board."
#endif

static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id  = DAC_CHANNEL_ID,
	.resolution  = DAC_RESOLUTION
};

void main(void)
{
	const struct device *dac_dev = device_get_binding(DAC_DEVICE_NAME);

	if (!dac_dev) {
		printk("Cannot get DAC device\n");
		return;
	}

	int ret = dac_channel_setup(dac_dev, &dac_ch_cfg);

	if (ret != 0) {
		printk("Setting up of DAC channel failed with code %d\n", ret);
		return;
	}

	printk("Generating sawtooth signal at DAC channel %d.\n",
		DAC_CHANNEL_ID);
	while (1) {
		/* Number of valid DAC values, e.g. 4096 for 12-bit DAC */
		const int dac_values = 1U << DAC_RESOLUTION;

		/*
		 * 1 msec sleep leads to about 4 sec signal period for 12-bit
		 * DACs. For DACs with lower resolution, sleep time needs to
		 * be increased.
		 * Make sure to sleep at least 1 msec even for future 16-bit
		 * DACs (lowering signal frequency).
		 */
		const int sleep_time = 4096 / dac_values > 0 ?
			4096 / dac_values : 1;

		for (int i = 0; i < dac_values; i++) {
			ret = dac_write_value(dac_dev, DAC_CHANNEL_ID, i);
			if (ret != 0) {
				printk("dac_write_value() failed with code %d\n", ret);
				return;
			}
			k_sleep(K_MSEC(sleep_time));
		}
	}
}

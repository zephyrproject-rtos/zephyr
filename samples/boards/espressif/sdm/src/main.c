/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "driver/sdm.h"

#define SDM_CHANNEL 0

static const struct device *const sdm_dev = DEVICE_DT_GET(DT_NODELABEL(sdm0));

int main(void)
{
	int8_t density = 0;
	int8_t step = 2;

	if (!device_is_ready(sdm_dev)) {
		printk("SDM device is not ready\n");
		return 0;
	}

	printk("Fading SDM channel %d\n", SDM_CHANNEL);

	while (true) {
		int ret;

		ret = esp32_sdm_set_pulse_density(sdm_dev, SDM_CHANNEL, density);
		if (ret != 0) {
			printk("set pulse density failed (%d)\n", ret);
			return 0;
		}

		k_msleep(10);

		if ((density >= 120) || (density <= -120)) {
			step = (int8_t)(-step);
		}

		density = (int8_t)(density + step);
	}

	return 0;
}

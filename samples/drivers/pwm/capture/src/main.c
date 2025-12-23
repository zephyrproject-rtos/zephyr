/*
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include "zephyr/drivers/pwm.h"

int main(void)
{
	int ret;
	uint64_t tim_clk_cycles;
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(capture));

	printk("PWM capture %s\n", CONFIG_BOARD_TARGET);
	if (!device_is_ready(dev)) {
		printk("device is not ready\n");
		return 0;
	}

	pwm_get_cycles_per_sec(dev, 0, &tim_clk_cycles);
	printk("timclk %llu Hz\n", tim_clk_cycles);

	while (1) {
		uint32_t period = 0;
		uint32_t width = 0;

		k_msleep(250);
		ret = pwm_capture_cycles(dev, 0,
					 (PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_SINGLE),
					 &period, &width, Z_TIMEOUT_MS(500));

		if (ret == -EBUSY) {
			pwm_disable_capture(dev, 0);
			continue;
		}

		if (ret) {
			printk("capture cycle err %d\n", ret);
			continue;
		}

		printk("{period:%u  pulse width: %u} in TIMCLK cycle\n",
			period,
			width);

		printk("{period: %f Hz duty: %f}\n",
			(double)tim_clk_cycles / (double)period,
			(double)(width * 100) / (double)period);
	}

	return 0;
}

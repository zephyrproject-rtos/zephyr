/*
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include "zephyr/drivers/pwm.h"
#define DT_DRV_COMPAT ti_mspm0g1x0x_g3x0x_timer_pwm

uint32_t gperiod;
uint32_t gwidth;
void pwm_capture_cb_handler(const struct device *dev,
			uint32_t channel,
			uint32_t period_cycles,
			uint32_t pulse_cycles,
			int status, void *user_data)
{
	gperiod = period_cycles;
	gwidth = pulse_cycles;
}

int main(void)
{
	int ret;
	uint64_t timclk_cycles;
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(capture));

	printf("PWM capture ! %s\n", CONFIG_BOARD_TARGET);
	if (!device_is_ready(dev)) {
		printf("device is not ready\n");
		return 0;
	}

	pwm_get_cycles_per_sec(dev, 0, &timclk_cycles);
	printk("\ntimclk %llu Hz\n", timclk_cycles);
	k_msleep(1);

	ret = pwm_configure_capture(dev, 0,
		 (PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_CONTINUOUS),
		 pwm_capture_cb_handler, NULL);

	printf("capture configure %d\n", ret);

	k_msleep(1);
	ret |= pwm_enable_capture(dev, 0);
	printf("capture enable %d\n", ret);

	while(1) {
		k_msleep(500);
		printk("{period:%u  pulse width: %u} in TIMCLK cycle\n",
			gperiod,
			gwidth);
		printk("{period: %u Hz duty: %u \% }\n",
			((uint32_t)timclk_cycles)/gperiod,
			(gwidth * 100)/gperiod);
	}

	return 0;
}

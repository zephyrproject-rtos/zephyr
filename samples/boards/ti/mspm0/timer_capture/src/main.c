/*
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include "zephyr/drivers/pwm.h"
#define DT_DRV_COMPAT ti_mspm0g1x0x_g3x0x_timer_pwm
#define CAPTURE_CONTINUOUS 0

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

#if CAPTURE_CONTINUOUS
	ret = pwm_configure_capture(dev, 0,
		 (PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_CONTINUOUS),
		 pwm_capture_cb_handler, NULL);

	printf("capture configure %d\n", ret);

	k_msleep(1);
	ret |= pwm_enable_capture(dev, 0);
	printf("capture enable %d\n", ret);
#endif
	while(1) {
		uint32_t period;
		uint32_t width;

		k_msleep(250);
#if CAPTURE_CONTINUOUS
		period = gperiod;
		width = gwidth;
#else
		ret = pwm_capture_cycles(dev, 0,
				 (PWM_CAPTURE_TYPE_BOTH | PWM_CAPTURE_MODE_SINGLE),
				 &period, &width, Z_TIMEOUT_MS(500));

		if (ret == -EBUSY) {
			pwm_disable_capture(dev, 0);
			continue;
		}

		if (ret) {
			printf("capture cycle err %d\n", ret);
			continue;
		}
#endif
		printk("{period:%u  pulse width: %u} in TIMCLK cycle\n",
			period,
			width);
		printk("{period: %f Hz duty: %f \% }\n",
			((float)timclk_cycles)/(float)period,
			(float)(width * 100)/(float)period);


	}
	return 0;
}

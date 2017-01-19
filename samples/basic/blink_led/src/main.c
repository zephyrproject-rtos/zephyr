/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM.
 *
 * This app uses PWM[0].
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <pwm.h>

#if defined(CONFIG_SOC_STM32F401XE) || defined(CONFIG_SOC_STM32L476XX)
#define PWM_DRIVER "PWM_2"
#define PWM_CHANNEL 1
#elif CONFIG_SOC_STM32F103XB
#define PWM_DRIVER "PWM_1"
#define PWM_CHANNEL 1
#else
#define PWM_DRIVER "PWM_0"
#define PWM_CHANNEL 0
#endif

/* in micro second */
#define MIN_PERIOD	2000

/* in micro second */
#define MAX_PERIOD	2000000

void main(void)
{
	struct device *pwm_dev;
	uint32_t period = MAX_PERIOD;
	uint8_t dir = 0;

	printk("PWM demo app-blink LED\n");

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
				     period, period / 2)) {
			printk("pwm pin set fails\n");
			return;
		}

		if (dir) {
			period *= 2;

			if (period > MAX_PERIOD) {
				dir = 0;
				period = MAX_PERIOD;
			}
		} else {
			period /= 2;

			if (period < MIN_PERIOD) {
				dir = 1;
				period = MIN_PERIOD;
			}
		}

		k_sleep(MSEC_PER_SEC * 4);
	}
}

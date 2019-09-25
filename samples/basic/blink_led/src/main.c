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
#include <sys/printk.h>
#include <device.h>
#include <drivers/pwm.h>

#if defined(DT_ALIAS_PWM_LED0_PWMS_CONTROLLER) && defined(DT_ALIAS_PWM_LED0_PWMS_CHANNEL)
/* get the defines from dt (based on alias 'pwm-led0') */
#define PWM_DRIVER	DT_ALIAS_PWM_LED0_PWMS_CONTROLLER
#define PWM_CHANNEL	DT_ALIAS_PWM_LED0_PWMS_CHANNEL
#else
#error "Choose supported PWM driver"
#endif

void main(void)
{
	struct device *pwm_dev;
	u32_t period, min_period, max_period;
	u64_t cycles;
	u8_t dir = 0U;

	printk("PWM demo app-blink LED\n");

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	/* Adjust max_period depending pwm capabilities */
	pwm_get_cycles_per_sec(pwm_dev, PWM_CHANNEL, &cycles);
	/* in very worst case, PWM has a 8 bit resolution and count up to 256 */
	period = max_period = 256 * USEC_PER_SEC / cycles;
	min_period = max_period / 64;

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
				     period, period / 2U)) {
			printk("pwm pin set fails\n");
			return;
		}

		if (dir) {
			period *= 2U;

			if (period > max_period) {
				dir = 0U;
				period = max_period;
			}
		} else {
			period /= 2U;

			if (period < min_period) {
				dir = 1U;
				period = min_period;
			}
		}

		k_sleep(MSEC_PER_SEC * 4U);
	}
}

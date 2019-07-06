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

/* in micro second */
#define MIN_PERIOD	(USEC_PER_SEC / 64U)

/* in micro second */
#define MAX_PERIOD	USEC_PER_SEC

void main(void)
{
	struct device *pwm_dev;
	u32_t period = MAX_PERIOD;
	u8_t dir = 0U;

	printk("PWM demo app-blink LED\n");

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
				     period, period / 2U)) {
			printk("pwm pin set fails\n");
			return;
		}

		if (dir) {
			period *= 2U;

			if (period > MAX_PERIOD) {
				dir = 0U;
				period = MAX_PERIOD;
			}
		} else {
			period /= 2U;

			if (period < MIN_PERIOD) {
				dir = 1U;
				period = MIN_PERIOD;
			}
		}

		k_sleep(MSEC_PER_SEC * 4U);
	}
}

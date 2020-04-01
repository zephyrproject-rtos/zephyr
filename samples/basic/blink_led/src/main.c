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
#ifdef DT_ALIAS_PWM_LED0_PWMS_FLAGS
#define PWM_FLAGS	DT_ALIAS_PWM_LED0_PWMS_FLAGS
#else
#define PWM_FLAGS	0
#endif
#else
#error "Choose supported PWM driver"
#endif

/* in microseconds */
#define MIN_PERIOD	(USEC_PER_SEC / 64U)

/* in microseconds */
#define MAX_PERIOD	USEC_PER_SEC

void main(void)
{
	struct device *pwm_dev;
	u32_t max_period;
	u32_t period;
	u8_t dir = 0U;

	printk("PWM demo app-blink LED\n");

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	/* In case the default MAX_PERIOD value cannot be set for some PWM
	 * hardware, try to decrease the value until it fits, but no further
	 * than to the value of MIN_PERIOD muliplied by four (to allow the
	 * sample to actually show some blinking with changing frequency).
	 */
	max_period = MAX_PERIOD;
	while (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
				max_period, max_period / 2U, PWM_FLAGS)) {
		max_period /= 2U;
		if (max_period < (4U * MIN_PERIOD)) {
			printk("This sample needs to set a period that is "
			       "not supported by the used PWM driver.");
			return;
		}
	}

	period = max_period;
	while (1) {
		if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
				     period, period / 2U, PWM_FLAGS)) {
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

			if (period < MIN_PERIOD) {
				dir = 1U;
				period = MIN_PERIOD;
			}
		}

		k_sleep(K_SECONDS(4U));
	}
}

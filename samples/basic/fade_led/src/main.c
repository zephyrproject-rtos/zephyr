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

#if DT_NODE_HAS_PROP(DT_ALIAS(pwm_led0), pwms) && \
    DT_PHA_HAS_CELL(DT_ALIAS(pwm_led0), pwms, channel)
/* get the defines from dt (based on alias 'pwm-led0') */
#define PWM_DRIVER	DT_PWMS_LABEL(DT_ALIAS(pwm_led0))
#define PWM_CHANNEL	DT_PWMS_CHANNEL(DT_ALIAS(pwm_led0))
#if DT_PHA_HAS_CELL(DT_ALIAS(pwm_led0), pwms, flags)
#define PWM_FLAGS	DT_PWMS_FLAGS(DT_ALIAS(pwm_led0))
#else
#define PWM_FLAGS	0
#endif
#else
#error "Choose supported PWM driver"
#endif

/*
 * 50 is flicker fusion threshold. Modulated light will be perceived
 * as steady by our eyes when blinking rate is at least 50.
 */
#define PERIOD (USEC_PER_SEC / 50U)

/* in micro second */
#define FADESTEP	2000

void main(void)
{
	struct device *pwm_dev;
	u32_t pulse_width = 0U;
	u8_t dir = 0U;

	printk("PWM demo app-fade LED\n");

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
					PERIOD, pulse_width, PWM_FLAGS)) {
			printk("pwm pin set fails\n");
			return;
		}

		if (dir) {
			if (pulse_width < FADESTEP) {
				dir = 0U;
				pulse_width = 0U;
			} else {
				pulse_width -= FADESTEP;
			}
		} else {
			pulse_width += FADESTEP;

			if (pulse_width >= PERIOD) {
				dir = 1U;
				pulse_width = PERIOD;
			}
		}

		k_sleep(K_SECONDS(1));
	}
}

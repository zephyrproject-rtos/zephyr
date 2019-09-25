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

#ifndef DT_ALIAS_PWM_0_LABEL
#error "Choose supported board or add new board for the application"
#endif

/*
 * Unlike pulse width, period is not a critical parameter for
 * motor control. 20ms is commonly used.
 */
#define PERIOD (USEC_PER_SEC / 50U)

/* all in micro second */
#define STEPSIZE 100
#define MINPULSEWIDTH 700
#define MAXPULSEWIDTH 2300

void main(void)
{
	struct device *pwm_dev;
	u32_t pulse_width = MINPULSEWIDTH;
	u8_t dir = 0U;

	printk("PWM demo app-servo control\n");

	pwm_dev = device_get_binding(DT_ALIAS_PWM_0_LABEL);
	if (!pwm_dev) {
		printk("Cannot find PWM device!\n");
		return;
	}

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, 0, PERIOD, pulse_width)) {
			printk("pwm pin set fails\n");
			return;
		}

		if (dir) {
			if (pulse_width <= MINPULSEWIDTH) {
				dir = 0U;
				pulse_width = MINPULSEWIDTH;
			} else {
				pulse_width -= STEPSIZE;
			}
		} else {
			pulse_width += STEPSIZE;

			if (pulse_width >= MAXPULSEWIDTH) {
				dir = 1U;
				pulse_width = MAXPULSEWIDTH;
			}
		}

		k_sleep(MSEC_PER_SEC);
	}
}

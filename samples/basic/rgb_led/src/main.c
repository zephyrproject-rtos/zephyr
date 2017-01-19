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

/*
 * 50 is flicker fusion threshold. Modulated light will be perceived
 * as steady by our eyes when blinking rate is at least 50.
 */
#define PERIOD (USEC_PER_SEC / 50)

/* in micro second */
#define STEPSIZE	2000

static int write_pin(struct device *pwm_dev, uint32_t pwm_pin,
		     uint32_t pulse_width)
{
	return pwm_pin_set_usec(pwm_dev, pwm_pin, PERIOD, pulse_width);
}

void main(void)
{
	struct device *pwm_dev;
	uint32_t pulse_width0, pulse_width1, pulse_width2;

	printk("PWM demo app-RGB LED\n");

	pwm_dev = device_get_binding("PWM_0");
	if (!pwm_dev) {
		printk("Cannot find PWM_0!\n");
		return;
	}

	while (1) {
		for (pulse_width0 = 0; pulse_width0 <= PERIOD;
		     pulse_width0 += STEPSIZE) {
			if (write_pin(pwm_dev, 0, pulse_width0) != 0) {
				printk("pin 0 write fails!\n");
				return;
			}

			for (pulse_width1 = 0; pulse_width1 <= PERIOD;
			     pulse_width1 += STEPSIZE) {
				if (write_pin(pwm_dev, 1, pulse_width1) != 0) {
					printk("pin 1 write fails!\n");
					return;
				}

				for (pulse_width2 = 0; pulse_width2 <= PERIOD;
				     pulse_width2 += STEPSIZE) {
					if (write_pin(pwm_dev, 2, pulse_width2)
					    != 0) {
						printk("pin 2 write fails!\n");
						return;
					}
					k_sleep(MSEC_PER_SEC);
				}
			}
		}
	}
}

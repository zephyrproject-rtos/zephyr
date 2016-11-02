/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#define FADESTEP	2000

void main(void)
{
	struct device *pwm_dev;
	uint32_t pulse_width = 0;
	uint8_t dir = 0;

	printk("PWM demo app-fade LED\n");

	pwm_dev = device_get_binding("PWM_0");
	if (!pwm_dev) {
		printk("Cannot find PWM_0!\n");
		return;
	}

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, 0, PERIOD, pulse_width)) {
			printk("pwm pin set fails\n");
			return;
		}

		if (dir) {
			if (pulse_width < FADESTEP) {
				dir = 0;
				pulse_width = 0;
			} else {
				pulse_width -= FADESTEP;
			}
		} else {
			pulse_width += FADESTEP;

			if (pulse_width >= PERIOD) {
				dir = 1;
				pulse_width = PERIOD;
			}
		}

		k_sleep(MSEC_PER_SEC);
	}
}

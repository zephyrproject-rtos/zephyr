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

/*
 * Copyright (c) 2015 Intel Corporation
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
 * @file Sample app to utilize PWM on Arduino 101.
 *
 * On x86 side of Arduino 101:
 * 1. PWM[0] is on IO3
 * 2. PWM[1] is on IO5
 * 3. PWM[2] is on IO7
 * 4. PWM[3] is on IO9
 *
 * The pwm_dw driver is being used.
 *
 * The sample runs like this on PWM[0] (IO3):
 * 1. At first, the period for the PWM is 1 second on, 1 second off.
 * 2. Every 4 seconds, the period is halved.
 * 3. After the period is faster than 1 ms on and 1 ms off, period
 *    starts to double.
 * 4. Every 4 seconds, period is doubled.
 * 5. When period is longer than 1 seconds, repeat from 2.
 *
 * If you have a LED connected, it should blink gradually faster
 * every 4 seconds. Then starts to blink slower. And repeat.
 */

#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <pwm.h>
#include <sys_clock.h>

#define HW_CLOCK_CYCLES_PER_USEC  (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / \
				   USEC_PER_SEC)

/**
 * unit: micro second.
 * 1 m sec for "on" or "off" time.
 */
#define MIN_PERIOD	1000

/**
 * unit: micro second.
 * 1 sec for "on" or "off" time
 */
#define MAX_PERIOD	1000000

#define SLEEPTICKS	SECONDS(4)

void main(void)
{
	struct nano_timer timer;
	void *timer_data[1];
	struct device *pwm_dev;
	uint32_t period;
	uint8_t dir;

	nano_timer_init(&timer, timer_data);

	printk("PWM demo app\n");

	pwm_dev = device_get_binding("PWM_0");
	if (!pwm_dev) {
		printk("Cannot find PWM_0!\n");
	}

	period = MAX_PERIOD * 2;
	dir = 0;

	while (1) {
		if (pwm_pin_set_period(pwm_dev, 0, period)) {
			printk("set period fails\n");
			return;
		}

		if (pwm_pin_set_values(pwm_dev, 0, 0,
				(period * HW_CLOCK_CYCLES_PER_USEC) / 2)) {
			printk("set values fails\n");
			return;
		}

		if (dir) {
			period *= 2;

			if (period > (MAX_PERIOD * 2)) {
				dir = 0;
				period = MAX_PERIOD * 2;
			}
		} else {
			period /= 2;

			if (period < (MIN_PERIOD * 2)) {
				dir = 1;
				period = MIN_PERIOD * 2;
			}
		}

		nano_timer_start(&timer, SLEEPTICKS);
		nano_timer_test(&timer, TICKS_UNLIMITED);
	}
}

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
 * 3. After the period is faster than 10ms, period starts to double.
 * 4. Every 4 seconds, period is doubled.
 * 5. When period is longer than 1 seconds, repeat from 2.
 *
 * If you have a LED connected, it should blink gradually faster
 * every 4 seconds. Then starts to blink slower. And repeat.
 */

#include <zephyr.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include <device.h>
#include <pwm.h>
#include <sys_clock.h>

/* about 1 ms */
#define MIN_PERIOD	320000

/* about 1 second */
#define MAX_PERIOD	32000000

#define SLEEPTICKS	SECONDS(4)

void main(void)
{
	struct nano_timer timer;
	void *timer_data[1];
	struct device *pwm_dev;
	uint32_t period;
	uint8_t dir;

	nano_timer_init(&timer, timer_data);

	PRINT("PWM_DW demo app\n");

	pwm_dev = device_get_binding(CONFIG_PWM_DW_DEV_NAME);
	if (!pwm_dev) {
		PRINT("Cannot find %s!\n", CONFIG_PWM_DW_DEV_NAME);
	}

	period = MAX_PERIOD;
	dir = 0;

	while (1) {
		pwm_pin_set_values(pwm_dev, 0, period, period);

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

		nano_timer_start(&timer, SLEEPTICKS);
		nano_timer_wait(&timer);
	}
}

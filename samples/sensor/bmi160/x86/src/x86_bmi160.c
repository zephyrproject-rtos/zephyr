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

#include <zephyr.h>
#include <sys_clock.h>
#include <stdio.h>
#include <device.h>
#include <sensor.h>
#include <misc/util.h>
#include <gpio.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>

QUARK_SE_IPM_DEFINE(bmi160_ipm, 0, QUARK_SE_IPM_OUTBOUND);

#define SLEEPTIME			MSEC(1000)

#define BMI160_INTERRUPT_PIN		4

struct device *ipm;

static void aon_gpio_callback(struct device *port, uint32_t pin)
{
	ipm_send(ipm, 0, 0, NULL, 0);
}

void main(void)
{
	uint32_t timer_data[2] = {0, 0};
	struct device *aon_gpio;
	struct nano_timer timer;

	nano_timer_init(&timer, timer_data);

	aon_gpio = device_get_binding("GPIO_1");
	if (!aon_gpio) {
		printf("aon_gpio device not found.\n");
		return;
	}

	ipm = device_get_binding("bmi160_ipm");
	if (!ipm) {
		printf("ipm device not found.\n");
		return;
	}

	gpio_set_callback(aon_gpio, aon_gpio_callback);
	gpio_pin_configure(aon_gpio, BMI160_INTERRUPT_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_pin_enable_callback(aon_gpio, BMI160_INTERRUPT_PIN);

	while (1) {
		nano_task_timer_start(&timer, SLEEPTIME);
		nano_task_timer_test(&timer, TICKS_UNLIMITED);
	}
}

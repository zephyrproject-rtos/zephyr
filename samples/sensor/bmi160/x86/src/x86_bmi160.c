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
#include <stdio.h>
#include <device.h>
#include <sensor.h>
#include <misc/util.h>
#include <gpio.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>

QUARK_SE_IPM_DEFINE(bmi160_ipm, 0, QUARK_SE_IPM_OUTBOUND);

#define SLEEPTIME                       1000

#define BMI160_INTERRUPT_PIN            4

struct device *ipm;
struct gpio_callback cb;

static void gpio_callback(struct device *port,
			  struct gpio_callback *cb, uint32_t pins)
{
	ipm_send(ipm, 0, 0, NULL, 0);
}

void main(void)
{
	struct device *gpio;

	gpio = device_get_binding("GPIO_1");
	if (!gpio) {
		printf("gpio device not found.\n");
		return;
	}

	ipm = device_get_binding("bmi160_ipm");
	if (!ipm) {
		printf("ipm device not found.\n");
		return;
	}

	gpio_init_callback(&cb, gpio_callback, BIT(BMI160_INTERRUPT_PIN));
	gpio_add_callback(gpio, &cb);

	gpio_pin_configure(gpio, BMI160_INTERRUPT_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_pin_enable_callback(gpio, BMI160_INTERRUPT_PIN);

	while (1) {
		k_sleep(SLEEPTIME);
	}
}

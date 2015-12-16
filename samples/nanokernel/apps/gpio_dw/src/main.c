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
 * @file Sample app to utilize GPIO on Arduino 101, x86 side.
 *
 * On x86 side of Arduino 101:
 * 1. GPIO_16 is on IO8
 * 2. GPIO_19 is on IO4
 *
 * The gpio_dw driver is being used.
 *
 * This sample app toggles GPIO_16/IO8. It also waits for
 * GPIO_19/IO4 to go high and display a message.
 *
 * If IO4 and IO8 are connected together, the GPIO should
 * triggers every 2 seconds. And you should see this repeatedly
 * on console:
 * "
 *     Toggling GPIO_16
 *     Toggling GPIO_16
 *     GPIO 19 triggered
 * "
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
#include <gpio.h>
#include <sys_clock.h>

#define SLEEPTICKS	SECONDS(1)

void gpio_callback(struct device *port, uint32_t pin)
{
	PRINT("GPIO %d triggered\n", pin);
}

void main(void)
{
	struct nano_timer timer;
	void *timer_data[1];
	struct device *gpio_dw;
	int ret;
	int toggle = 1;

	nano_timer_init(&timer, timer_data);

	gpio_dw = device_get_binding(CONFIG_GPIO_DW_0_NAME);
	if (!gpio_dw) {
		PRINT("Cannot find GPIO_DW_0!\n");
	}

	/* Setup GPIO_16/IO8 to be output */
	ret = gpio_pin_configure(gpio_dw, 16, (GPIO_DIR_OUT));
	if (ret) {
		PRINT("Error configuring GPIO_16!\n");
	}

	/* Setup GPIO_19/IO4 to be input,
	 * and triggers on rising edge.
	 */
	ret = gpio_pin_configure(gpio_dw, 19,
			(GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
			 | GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE));
	if (ret) {
		PRINT("Error configuring GPIO_16!\n");
	}

	ret = gpio_set_callback(gpio_dw, gpio_callback);
	if (ret) {
		PRINT("Cannot setup callback!\n");
	}

	ret = gpio_pin_enable_callback(gpio_dw, 19);
	if (ret) {
		PRINT("Error enabling callback!\n");
	}

	while (1) {
		PRINT("Toggling GPIO_16\n");

		ret = gpio_pin_write(gpio_dw, 16, toggle);
		if (ret) {
			PRINT("Error set GPIO_16!\n");
		}

		if (toggle) {
			toggle = 0;
		} else {
			toggle = 1;
		}

		nano_timer_start(&timer, SLEEPTICKS);
		nano_timer_wait(&timer);
	}
}

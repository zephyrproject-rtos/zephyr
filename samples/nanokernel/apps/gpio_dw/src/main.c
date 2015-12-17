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
 * @file Sample app to utilize GPIO on Arduino 101.
 *
 * x86
 * --------------------
 *
 * On x86 side of Arduino 101:
 * 1. GPIO_16 is on IO8
 * 2. GPIO_19 is on IO4

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
 *     GPIO_19 triggered
 * "
 *
 * Sensor Subsystem
 * --------------------
 *
 * On Sensor Subsystem of Arduino 101:
 * 1. GPIO_SS[ 2] is on A00
 * 2. GPIO_SS[ 3] is on A01
 * 3. GPIO_SS[ 4] is on A02
 * 4. GPIO_SS[ 5] is on A03
 * 5. GPIO_SS[10] is on IO3
 * 6. GPIO_SS[11] is on IO5
 * 7. GPIO_SS[12] is on IO6
 * 8. GPIO_SS[13] is on IO9
 *
 * There are two 8-bit GPIO controllers on the sensor subsystem.
 * GPIO_SS[0-7] are on GPIO_0 while GPIO_SS[8-15] are on GPIO_1.
 *
 * The gpio_dw driver is being used. The first GPIO controller
 * is mapped to device "GPIO_0", and the second GPIO controller
 * is mapped to device "GPIO_1".
 *
 * This sample app toggles GPIO_SS_2/A0. It also waits for
 * GPIO_SS_3/A1 to go high and display a message.
 *
 * If A0 and A1 are connected together, the GPIO should
 * triggers every 2 seconds. And you should see this repeatedly
 * on console (via ipm_console0):
 * "
 *     Toggling GPIO_SS_2
 *     Toggling GPIO_SS_2
 *     GPIO_SS_3 triggered
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

#ifdef CONFIG_PLATFORM_QUARK_SE_SS
#define GPIO_OUT_PIN	2
#define GPIO_INT_PIN	3
#define GPIO_NAME	"GPIO_SS_"
#else
#define GPIO_OUT_PIN	16
#define GPIO_INT_PIN	19
#define GPIO_NAME	"GPIO_"
#endif

void gpio_callback(struct device *port, uint32_t pin)
{
	PRINT(GPIO_NAME "%d triggered\n", pin);
}

void main(void)
{
	struct nano_timer timer;
	void *timer_data[1];
	struct device *gpio_dev;
	int ret;
	int toggle = 1;

	nano_timer_init(&timer, timer_data);

	gpio_dev = device_get_binding(CONFIG_GPIO_DW_0_NAME);
	if (!gpio_dev) {
		PRINT("Cannot find %s!\n", CONFIG_GPIO_DW_0_NAME);
	}

	/* Setup GPIO output */
	ret = gpio_pin_configure(gpio_dev, GPIO_OUT_PIN, (GPIO_DIR_OUT));
	if (ret) {
		PRINT("Error configuring " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
	}

	/* Setup GPIO input, and triggers on rising edge. */
	ret = gpio_pin_configure(gpio_dev, GPIO_INT_PIN,
			(GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
			 | GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE));
	if (ret) {
		PRINT("Error configuring " GPIO_NAME "%d!\n", GPIO_INT_PIN);
	}

	ret = gpio_set_callback(gpio_dev, gpio_callback);
	if (ret) {
		PRINT("Cannot setup callback!\n");
	}

	ret = gpio_pin_enable_callback(gpio_dev, GPIO_INT_PIN);
	if (ret) {
		PRINT("Error enabling callback!\n");
	}

	while (1) {
		PRINT("Toggling " GPIO_NAME "%d\n", GPIO_OUT_PIN);

		ret = gpio_pin_write(gpio_dev, GPIO_OUT_PIN, toggle);
		if (ret) {
			PRINT("Error set " GPIO_NAME "%d!\n", GPIO_OUT_PIN);
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

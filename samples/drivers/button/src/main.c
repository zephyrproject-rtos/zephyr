/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
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
#include <device.h>
#include <gpio.h>
#include <misc/util.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

/* change this to use another GPIO port */
#define PORT	"GPIOC"
/* change this to use another GPIO pin */
#define PIN	13
/* change this to enable pull-up/pull-down */
#define PULL_UP 0
/* change this to use a different interrupt trigger */
#define EDGE    (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)


void button_pressed(struct device *gpiob,
		    struct gpio_callback *cb, uint32_t pins)
{
	PRINT("Button pressed at %d\n", sys_tick_get_32());
}

static struct gpio_callback gpio_cb;

void main(void)
{
	struct device *gpiob;

	gpiob = device_get_binding(PORT);

	gpio_pin_configure(gpiob, PIN,
			GPIO_DIR_IN | GPIO_INT
			| EDGE
			| PULL_UP);

	gpio_init_callback(&gpio_cb, button_pressed, BIT(PIN));

	gpio_add_callback(gpiob, &gpio_cb);
	gpio_pin_enable_callback(gpiob, PIN);

	while (1) {
		int val = 0;

		gpio_pin_read(gpiob, PIN, &val);
		PRINT("GPIO val: %d\n", val);
		task_sleep(MSEC(500));
	}
}

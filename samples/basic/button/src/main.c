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
#include <board.h>
#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <misc/printk.h>

/* change this to use another GPIO port */
#ifdef SW0_GPIO_NAME
#define PORT	SW0_GPIO_NAME
#else
#error SW0_GPIO_NAME needs to be set in board.h
#endif

/* change this to use another GPIO pin */
#ifdef SW0_GPIO_PIN
#define PIN     SW0_GPIO_PIN
#else
#error SW0_GPIO_PIN needs to be set in board.h
#endif

/* change this to enable pull-up/pull-down */
#define PULL_UP 0

/* change this to use a different interrupt trigger */
#define EDGE    (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)


/* Sleep time */
#define SLEEP_TIME	500


void button_pressed(struct device *gpiob, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %d\n", k_cycle_get_32());
}

static struct gpio_callback gpio_cb;

void main(void)
{
	struct device *gpiob;

	printk("Press the user defined button on the board\n");
	gpiob = device_get_binding(PORT);
	if (!gpiob) {
		printk("error\n");
	}

	gpio_pin_configure(gpiob, PIN,
			   GPIO_DIR_IN | GPIO_INT |  PULL_UP | EDGE);

	gpio_init_callback(&gpio_cb, button_pressed, BIT(PIN));

	gpio_add_callback(gpiob, &gpio_cb);
	gpio_pin_enable_callback(gpiob, PIN);

	while (1) {
		uint32_t val = 0;

		gpio_pin_read(gpiob, PIN, &val);
		k_sleep(SLEEP_TIME);
	}
}

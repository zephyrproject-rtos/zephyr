/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>

#include "publisher.h"

struct device *led_device[4];
struct device *button_device[4];

K_WORK_DEFINE(button_work, publish);

static void button_pressed(struct device *dev,
			   struct gpio_callback *cb, u32_t pins)
{
	k_work_submit(&button_work);
}

void app_gpio_init(void)
{
	static struct gpio_callback button_cb[4];

	/* LEDs configuration & setting */

	led_device[0] = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	gpio_pin_configure(led_device[0], DT_ALIAS_LED0_GPIOS_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[0], DT_ALIAS_LED0_GPIOS_PIN, 1);

	led_device[1] = device_get_binding(DT_ALIAS_LED1_GPIOS_CONTROLLER);
	gpio_pin_configure(led_device[1], DT_ALIAS_LED1_GPIOS_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[1], DT_ALIAS_LED1_GPIOS_PIN, 1);

	led_device[2] = device_get_binding(DT_ALIAS_LED2_GPIOS_CONTROLLER);
	gpio_pin_configure(led_device[2], DT_ALIAS_LED2_GPIOS_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[2], DT_ALIAS_LED2_GPIOS_PIN, 1);

	led_device[3] = device_get_binding(DT_ALIAS_LED3_GPIOS_CONTROLLER);
	gpio_pin_configure(led_device[3], DT_ALIAS_LED3_GPIOS_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[3], DT_ALIAS_LED3_GPIOS_PIN, 1);

	/* Buttons configuration & setting */

	k_work_init(&button_work, publish);

	button_device[0] = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);
	gpio_pin_configure(button_device[0], DT_ALIAS_SW0_GPIOS_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[0], button_pressed, BIT(DT_ALIAS_SW0_GPIOS_PIN));
	gpio_add_callback(button_device[0], &button_cb[0]);
	gpio_pin_enable_callback(button_device[0], DT_ALIAS_SW0_GPIOS_PIN);

	button_device[1] = device_get_binding(DT_ALIAS_SW1_GPIOS_CONTROLLER);
	gpio_pin_configure(button_device[1], DT_ALIAS_SW1_GPIOS_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[1], button_pressed, BIT(DT_ALIAS_SW1_GPIOS_PIN));
	gpio_add_callback(button_device[1], &button_cb[1]);
	gpio_pin_enable_callback(button_device[1], DT_ALIAS_SW1_GPIOS_PIN);

	button_device[2] = device_get_binding(DT_ALIAS_SW2_GPIOS_CONTROLLER);
	gpio_pin_configure(button_device[2], DT_ALIAS_SW2_GPIOS_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[2], button_pressed, BIT(DT_ALIAS_SW2_GPIOS_PIN));
	gpio_add_callback(button_device[2], &button_cb[2]);
	gpio_pin_enable_callback(button_device[2], DT_ALIAS_SW2_GPIOS_PIN);

	button_device[3] = device_get_binding(DT_ALIAS_SW3_GPIOS_CONTROLLER);
	gpio_pin_configure(button_device[3], DT_ALIAS_SW3_GPIOS_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_PUD_PULL_UP |
			    GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[3], button_pressed, BIT(DT_ALIAS_SW3_GPIOS_PIN));
	gpio_add_callback(button_device[3], &button_cb[3]);
	gpio_pin_enable_callback(button_device[3], DT_ALIAS_SW3_GPIOS_PIN);
}

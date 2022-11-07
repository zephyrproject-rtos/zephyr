/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "app_gpio.h"
#include "publisher.h"

K_WORK_DEFINE(button_work, publish);

static void button_pressed(const struct device *dev,
			   struct gpio_callback *cb, uint32_t pins)
{
	k_work_submit(&button_work);
}

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)

#ifdef ONE_LED_ONE_BUTTON_BOARD
#define NUM_LED_NUM_BUTTON 1
#else
#define NUM_LED_NUM_BUTTON 4
#endif

const struct gpio_dt_spec led_device[NUM_LED_NUM_BUTTON] = {
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),
#ifndef ONE_LED_ONE_BUTTON_BOARD
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
#endif
};

const struct gpio_dt_spec button_device[NUM_LED_NUM_BUTTON] = {
	GPIO_DT_SPEC_GET(SW0_NODE, gpios),
#ifndef ONE_LED_ONE_BUTTON_BOARD
	GPIO_DT_SPEC_GET(SW1_NODE, gpios),
	GPIO_DT_SPEC_GET(SW2_NODE, gpios),
	GPIO_DT_SPEC_GET(SW3_NODE, gpios),
#endif
};

void app_gpio_init(void)
{
	static struct gpio_callback button_cb[4];
	int i;

	/* LEDs configuration & setting */

	for (i = 0; i < ARRAY_SIZE(led_device); i++) {
		if (!device_is_ready(led_device[i].port)) {
			return;
		}
		gpio_pin_configure_dt(&led_device[i], GPIO_OUTPUT_INACTIVE);
	}

	/* Buttons configuration & setting */

	k_work_init(&button_work, publish);

	for (i = 0; i < ARRAY_SIZE(button_device); i++) {
		if (!device_is_ready(button_device[i].port)) {
			return;
		}
		gpio_pin_configure_dt(&button_device[i], GPIO_INPUT);
		gpio_pin_interrupt_configure_dt(&button_device[i],
						GPIO_INT_EDGE_TO_ACTIVE);
		gpio_init_callback(&button_cb[i], button_pressed,
				   BIT(button_device[i].pin));
		gpio_add_callback(button_device[i].port, &button_cb[i]);
	}
}

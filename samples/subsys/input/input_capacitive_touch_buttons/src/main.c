/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_mchp_touch_api.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* LED on (or) off value */
#define LED_ON  1
#define LED_OFF 0

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE  DT_ALIAS(led0)
#define TOUCH_NODE DT_NODELABEL(ptc)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static void touch_callback(struct input_event *evt, void *user_data)
{
	uint8_t no_of_sensors = get_def_no_of_sensors(DEVICE_DT_GET(TOUCH_NODE));

	if (evt != NULL) {
		if (evt->code == INPUT_BTN_TOUCH) {
			for (uint8_t button = 0u; button < no_of_sensors; button++) {
				if (get_sensor_state(evt->dev, button) == QTM_KEY_STATE_DETECT) {
					gpio_pin_set_dt(&led0, LED_ON);
				} else {
					gpio_pin_set_dt(&led0, LED_OFF);
				}
			}
		}
	}
}

INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(TOUCH_NODE), touch_callback, NULL, button);

int main(void)
{
	gpio_is_ready_dt(&led0);

	/* turn off the led by default */
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/* user code */
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}

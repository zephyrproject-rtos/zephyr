/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios,
						     {0});

#define SLEEP_TIME_MS	1

int main(void)
{
	int ret = 0;

	if (led.port && !gpio_is_ready_dt(&led)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n",
		       ret, led.port->name);
		led.port = NULL;
	}
	if (led.port) {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led.port->name, led.pin);
			led.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", led.port->name, led.pin);
		}
	}


// #ifdef CONFIG_SAMPLE_DO_OUTPUT
	printk("0123456789Hello World from minimal hihi!\n\r");
// #endif
	if (led.port) {
		while (1) {
			/* If we have an LED, match its state to the button's. */
			int val = 1;

			if (val >= 0) {
				gpio_pin_set_dt(&led, val);
			}
            printk("0");
			k_msleep(SLEEP_TIME_MS*1000);
            gpio_pin_set_dt(&led, 0);
            k_msleep(SLEEP_TIME_MS*1000);
		}
	}

	return 0;
}

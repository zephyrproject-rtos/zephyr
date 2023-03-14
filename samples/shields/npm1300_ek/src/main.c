/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/npm1300.h>
#include <zephyr/sys/printk.h>
#include <getopt.h>

#define SLEEP_TIME_MS 100

static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static const struct device *regulators = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));

void configure_ui(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button1)) {
		printk("Error: button device %s is not ready\n", button1.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button1.port->name,
		       button1.pin);
		return;
	}

	printk("Set up button at %s pin %d\n", button1.port->name, button1.pin);
}

void main(void)
{
	configure_ui();

	if (!device_is_ready(regulators)) {
		printk("Error: Regulator device is not ready\n");
		return;
	}

	while (1) {
		/* Cycle regulator control GPIOs when first button pressed */
		static bool last_button;
		static int dvs_state;
		bool button_state = gpio_pin_get_dt(&button1) == 1;

		if (button_state && !last_button) {
			dvs_state = (dvs_state + 1) % 4;
			regulator_parent_dvs_state_set(regulators, dvs_state);
		}

		last_button = button_state;
		k_msleep(SLEEP_TIME_MS);
	}
}

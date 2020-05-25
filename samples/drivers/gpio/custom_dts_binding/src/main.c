/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>

#define LOAD_SWITCH DT_NODELABEL(load_switch)

#if DT_NODE_EXISTS(LOAD_SWITCH)
#define LOAD_SWITCH_PORT	DT_GPIO_LABEL(LOAD_SWITCH, gpios)
#define LOAD_SWITCH_PIN		DT_GPIO_PIN(LOAD_SWITCH, gpios)
#define LOAD_SWITCH_FLAGS	DT_GPIO_FLAGS(LOAD_SWITCH, gpios)
#else
#error "Overlay for power output node not properly defined."
#endif

void main(void)
{
	struct device *switch_dev = device_get_binding(LOAD_SWITCH_PORT);

	printk("Initializing pin %d on port %s with inactive level.\n",
		LOAD_SWITCH_PIN, LOAD_SWITCH_PORT);

	gpio_pin_configure(switch_dev, LOAD_SWITCH_PIN,
		LOAD_SWITCH_FLAGS | GPIO_OUTPUT_INACTIVE);

	printk("Waiting one second.\n");

	k_sleep(K_MSEC(1000));

	printk("Setting pin to active level.\n");

	gpio_pin_set(switch_dev, LOAD_SWITCH_PIN, 1);
}

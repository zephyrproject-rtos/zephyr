/*
 * Copyright (c) 2020 Circuit Dojo LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

#define GPIO0 DT_LABEL(DT_NODELABEL(gpio0))
#define POWER_LATCH_PIN 31

static int board_sparkfun_thing_plus_nrf9160_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Get the device binding */
	const struct device *gpio = device_get_binding(GPIO0);

	/* Return if NULL */
	if (gpio == NULL) {
		return -ENODEV;
	}

	/* Configure latch pin as output. */
	gpio_pin_configure(gpio, POWER_LATCH_PIN, GPIO_OUTPUT_HIGH);

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(board_sparkfun_thing_plus_nrf9160_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

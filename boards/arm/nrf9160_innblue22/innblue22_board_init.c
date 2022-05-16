/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

#define VDD_5V0_PWR_CTRL_GPIO_PIN  21   /* ENABLE_5V0_BOOST  --> speed sensor */

static int pwr_ctrl_init(const struct device *dev)
{
	const struct device *gpio;
	int    err = -ENODEV;

	/* Get handle of the GPIO device. */
	gpio = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));

	/* Valid handle? */
	if (gpio != NULL) {

		/* Configure this pin as output. */
		err = gpio_pin_configure(gpio, VDD_5V0_PWR_CTRL_GPIO_PIN,
							GPIO_OUTPUT_ACTIVE);
		if (err == 0) {

			/* Write "1" to this pin. */
			err = gpio_pin_set(gpio, VDD_5V0_PWR_CTRL_GPIO_PIN, 1);
		}

		/* Wait for the rail to come up and stabilize. */
		k_sleep(K_MSEC(10));
	}

	/* Operation status? */
	return (err);
}

SYS_INIT(pwr_ctrl_init, POST_KERNEL, 70);

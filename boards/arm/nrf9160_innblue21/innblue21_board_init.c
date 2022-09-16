/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

#define VDD_3V3_PWR_CTRL_GPIO_PIN  12   /* ENABLE_3V3_SENSOR --> i2c sensors  */
#define VDD_5V0_PWR_CTRL_GPIO_PIN  21   /* ENABLE_5V0_BOOST  --> speed sensor */

/* Configures the pin as output and sets them high. */
static void config_pin(const struct device *gpio, int pin)
{
	int err;

	/* Configure this pin as output. */
	err = gpio_pin_configure(gpio, pin, GPIO_OUTPUT_ACTIVE);
	if (err == 0) {
		/* Write "1" to this pin. */
		err = gpio_pin_set(gpio, pin, 1);
	}

	/* Wait for the rail to come up and stabilize. */
	k_sleep(K_MSEC(10));
}

static int pwr_ctrl_init(const struct device *dev)
{
	const struct device *gpio;

	/* Get handle of the GPIO device. */
	gpio = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));

	/* Valid handle? */
	if (gpio != NULL) {
		/* Configure the gpio pin. */
		config_pin(gpio, VDD_3V3_PWR_CTRL_GPIO_PIN);
		config_pin(gpio, VDD_5V0_PWR_CTRL_GPIO_PIN);
	}

	return 0;
}

SYS_INIT(pwr_ctrl_init, POST_KERNEL, 70);

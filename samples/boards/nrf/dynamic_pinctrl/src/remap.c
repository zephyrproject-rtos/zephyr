/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>

/* make sure devices and remap hook are initialized in the correct order */
BUILD_ASSERT((CONFIG_GPIO_INIT_PRIORITY < CONFIG_REMAP_INIT_PRIORITY) &&
	     (CONFIG_REMAP_INIT_PRIORITY < CONFIG_SERIAL_INIT_PRIORITY),
	     "Device driver priorities are not set correctly");

PINCTRL_DT_DEV_CONFIG_DECLARE(DT_NODELABEL(uart0));

/* UART0 alternative configurations (default and sleep states) */
PINCTRL_DT_STATE_PINS_DEFINE(DT_PATH(zephyr_user), uart0_alt_default);
#ifdef CONFIG_PM_DEVICE
PINCTRL_DT_STATE_PINS_DEFINE(DT_PATH(zephyr_user), uart0_alt_sleep);
#endif

static const struct pinctrl_state uart0_alt[] = {
	PINCTRL_DT_STATE_INIT(uart0_alt_default, PINCTRL_STATE_DEFAULT),
#ifdef CONFIG_PM_DEVICE
	PINCTRL_DT_STATE_INIT(uart0_alt_sleep, PINCTRL_STATE_SLEEP),
#endif
};

static int remap_pins(void)
{
	int ret;
	const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0),
							       gpios, {0});


	if (!gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	/* remap UART0 pins if button is pressed */
	if (gpio_pin_get_dt(&button)) {
		struct pinctrl_dev_config *uart0_config =
			PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(uart0));

		return pinctrl_update_states(uart0_config, uart0_alt,
					     ARRAY_SIZE(uart0_alt));

	}

	return 0;
}

SYS_INIT(remap_pins, PRE_KERNEL_1, CONFIG_REMAP_INIT_PRIORITY);

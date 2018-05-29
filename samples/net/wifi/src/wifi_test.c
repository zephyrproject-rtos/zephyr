/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>

#ifdef CONFIG_WIFI_WINC1500

#include <device.h>
#include <gpio.h>
#include <wifi/winc1500.h>

static
struct winc1500_gpio_configuration winc1500_gpios[WINC1500_GPIO_IDX_MAX] = {
	{ .dev = NULL, .pin = CONFIG_WINC1500_GPIO_CHIP_EN },
	{ .dev = NULL, .pin = CONFIG_WINC1500_GPIO_IRQN },
	{ .dev = NULL, .pin = CONFIG_WINC1500_GPIO_RESET_N },
};

struct winc1500_gpio_configuration *winc1500_configure_gpios(void)
{
	const int flags_int_in = (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE |
				  GPIO_INT_EDGE);
	const int flags_noint_out = GPIO_DIR_OUT;
	struct device *gpio;

	gpio = device_get_binding(CONFIG_WINC1500_GPIO_DRV_NAME);

	gpio_pin_configure(gpio, winc1500_gpios[WINC1500_GPIO_IDX_CHIP_EN].pin,
			   flags_noint_out);
	gpio_pin_configure(gpio, winc1500_gpios[WINC1500_GPIO_IDX_IRQN].pin,
			   flags_int_in);
	gpio_pin_configure(gpio, winc1500_gpios[WINC1500_GPIO_IDX_RESET_N].pin,
			   flags_noint_out);

	winc1500_gpios[WINC1500_GPIO_IDX_CHIP_EN].dev = gpio;
	winc1500_gpios[WINC1500_GPIO_IDX_IRQN].dev = gpio;
	winc1500_gpios[WINC1500_GPIO_IDX_RESET_N].dev = gpio;

	return winc1500_gpios;
}

#endif /* CONFIG_WIFI_WINC1500 */

void main(void)
{
	k_sleep(K_FOREVER);
}

/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

#define IP101GRI_RESET_N_PIN	5

static int board_esp32_ethernet_kit_init(void)
{
	const struct device *gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));

	if (!device_is_ready(gpio)) {
		return -ENODEV;
	}

	/* Enable the Ethernet phy */
	int res = gpio_pin_configure(
		gpio, IP101GRI_RESET_N_PIN,
		GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH);

	return res;
}

SYS_INIT(board_esp32_ethernet_kit_init, PRE_KERNEL_2, CONFIG_GPIO_INIT_PRIORITY);

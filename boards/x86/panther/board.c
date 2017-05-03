/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include "board.h"
#include <uart.h>
#include <device.h>
#include <init.h>
#include <gpio.h>

#if CONFIG_SERIAL
static int uart_switch(struct device *port)
{
	ARG_UNUSED(port);

	struct device *gpio;

	gpio = device_get_binding(CONFIG_GPIO_QMSI_1_NAME);
	gpio_pin_configure(gpio, UART_CONSOLE_SWITCH, GPIO_DIR_OUT);
	gpio_pin_write(gpio, UART_CONSOLE_SWITCH, 0);

	return 0;
}

SYS_INIT(uart_switch, POST_KERNEL, CONFIG_UART_CONSOLE_INIT_PRIORITY);
#endif

#if defined(CONFIG_WIFI_WINC1500)

static struct device *winc1500_gpio_config[WINC1500_GPIO_IDX_LAST_ENTRY];

struct device **winc1500_configure_gpios(void)
{
	struct device *gpio;
	const int flags_noint_out = GPIO_DIR_OUT;

	gpio = device_get_binding(CONFIG_WINC1500_GPIO_1_NAME);

	gpio_pin_configure(gpio, CONFIG_WINC1500_GPIO_RESET_N, flags_noint_out);
	winc1500_gpio_config[WINC1500_GPIO_IDX_RESET_N] = gpio;

	gpio = device_get_binding(CONFIG_WINC1500_GPIO_0_NAME);

	gpio_pin_configure(gpio, CONFIG_WINC1500_GPIO_CHIP_EN, flags_noint_out);
	winc1500_gpio_config[WINC1500_GPIO_IDX_CHIP_EN] = gpio;

	gpio_pin_configure(gpio, CONFIG_WINC1500_GPIO_WAKE, flags_noint_out);
	winc1500_gpio_config[WINC1500_GPIO_IDX_WAKE] = gpio;

	return winc1500_gpio_config;
}

void winc1500_configure_intgpios(void)
{
	struct device *gpio;

	const int flags_int_in = (GPIO_DIR_IN | GPIO_INT |
				  GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE |
				  GPIO_INT_EDGE);

	gpio = device_get_binding(CONFIG_WINC1500_GPIO_0_NAME);

	gpio_pin_configure(gpio, CONFIG_WINC1500_GPIO_IRQN, flags_int_in);
	winc1500_gpio_config[WINC1500_GPIO_IDX_IRQN] = gpio;

}
#endif /* CONFIG_WIFI_WINC1500 */

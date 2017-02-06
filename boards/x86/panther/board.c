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

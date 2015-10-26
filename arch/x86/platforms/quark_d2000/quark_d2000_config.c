/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Board config file for Quark D2000
 */

#include <device.h>
#include <init.h>

#include <nanokernel.h>

#include "board.h"

#ifdef CONFIG_NS16550
#include <uart.h>
#include <console/uart_console.h>
#include <serial/ns16550.h>

#if defined(CONFIG_UART_CONSOLE)
/**
 * @brief Initialize NS16550 serial port #1
 *
 * UART #1 is being used as console. So initialize it
 * for console I/O.
 *
 * @param dev The UART device struct
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static int ns16550_uart_console_init(struct device *dev)
{
	struct uart_init_info info = {
		.baud_rate = CONFIG_UART_CONSOLE_BAUDRATE,
		.sys_clk_freq = UART_XTAL_FREQ,
		.irq_pri = CONFIG_UART_CONSOLE_INT_PRI
	};

	/* enable clock gating */
#if (CONFIG_UART_CONSOLE_INDEX == 0)
	sys_set_bit(CLOCK_PERIPHERAL_BASE_ADDR, 17);
#elif (CONFIG_UART_CONSOLE_INDEX == 1)
	sys_set_bit(CLOCK_PERIPHERAL_BASE_ADDR, 18);
#endif
	sys_set_bit(CLOCK_PERIPHERAL_BASE_ADDR, 1);
	uart_init(UART_CONSOLE_DEV, &info);

	return DEV_OK;
}

#endif /* CONFIG_UART_CONSOLE */

struct uart_device_config ns16550_uart_dev_cfg[] = {
	{
		.port = CONFIG_UART0_CONSOLE_REGS,
		.irq = CONFIG_UART0_CONSOLE_IRQ,
		.irq_pri = CONFIG_UART0_CONSOLE_INT_PRI,

		.port_init = ns16550_uart_port_init,

#if defined(CONFIG_UART_CONSOLE)
		.config_func = ns16550_uart_console_init,
#endif
	},
	{
		.port = CONFIG_UART1_CONSOLE_REGS,
		.irq = CONFIG_UART1_CONSOLE_IRQ,
		.irq_pri = CONFIG_UART1_CONSOLE_INT_PRI,

		.port_init = ns16550_uart_port_init,

#if defined(CONFIG_UART_CONSOLE)
		.config_func = ns16550_uart_console_init,
#endif
	},
};

struct uart_ns16550_dev_data_t ns16550_uart_dev_data[2];


/* UART 0 */
DECLARE_DEVICE_INIT_CONFIG(ns16550_uart0,
			   CONFIG_UART_PORT_0_NAME,
			   &uart_platform_init,
			   &ns16550_uart_dev_cfg[0]);

SYS_DEFINE_DEVICE(ns16550_uart0, &ns16550_uart_dev_data[0],
					SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);


/* UART 1 */
DECLARE_DEVICE_INIT_CONFIG(ns16550_uart1,
			   CONFIG_UART_PORT_1_NAME,
			   &uart_platform_init,
			   &ns16550_uart_dev_cfg[1]);

SYS_DEFINE_DEVICE(ns16550_uart1, &ns16550_uart_dev_data[1],
					SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);


struct device * const uart_devs[] = {
	&__initconfig_ns16550_uart0,
	&__initconfig_ns16550_uart1,
};

#endif

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
 * @file Contains configuration for galileo platforms.
 */

#include <stdio.h>
#include <stdint.h>
#include <device.h>
#include <init.h>

#include "board.h"

#ifdef CONFIG_NS16550
#include <uart.h>
#include <bluetooth/uart.h>
#include <console/uart_console.h>
#include <serial/ns16550.h>


#if defined(CONFIG_UART_CONSOLE) || defined(CONFIG_BLUETOOTH_UART)
/**
 * @brief Initialize NS16550 serial port #1
 *
 * @param dev The UART device struct
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static int ns16550_uart_init(struct device *dev)
{
#if defined(CONFIG_UART_CONSOLE)
	struct uart_init_info info = {
		.baud_rate = CONFIG_UART_CONSOLE_BAUDRATE,
		.sys_clk_freq = UART_XTAL_FREQ,
		.irq_pri = CONFIG_UART_CONSOLE_INT_PRI
	};

	if (dev == UART_CONSOLE_DEV) {
		uart_init(UART_CONSOLE_DEV, &info);
	}
#endif

	return DEV_OK;
}
#else
static int ns16550_uart_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_OK;
}
#endif /* CONFIG_UART_CONSOLE || CONFIG_BLUETOOTH_UART */

/**
 * @brief UART Device configuration.
 *
 * This contains the device configuration for UART.
 */
struct uart_device_config ns16550_uart_dev_cfg[] = {
	{
		.port = CONFIG_NS16550_PORT_0_BASE_ADDR,

		.port_init = ns16550_uart_port_init,
#if defined(CONFIG_UART_CONSOLE) || defined(CONFIG_BLUETOOTH_UART)
		.config_func = ns16550_uart_init,
		.irq = CONFIG_UART_CONSOLE_IRQ,
#endif
		.pci_dev.class = PCI_CLASS_COMM_CTLR,
		.pci_dev.bus = CONFIG_UART_PCI_BUS,
		.pci_dev.dev = CONFIG_UART_PCI_DEV,
		.pci_dev.vendor_id = CONFIG_UART_PCI_VENDOR_ID,
		.pci_dev.device_id = CONFIG_UART_PCI_DEVICE_ID,
		.pci_dev.function = CONFIG_UART_PORT_0_FUNCTION,
		.pci_dev.bar = CONFIG_UART_PCI_BAR,
	},
	{
		.port = CONFIG_NS16550_PORT_1_BASE_ADDR,
		.irq = CONFIG_UART_PORT_1_IRQ,
		.irq_pri = CONFIG_UART_PORT_1_IRQ_PRIORITY,

		.port_init = ns16550_uart_port_init,
#if defined(CONFIG_UART_CONSOLE) || defined(CONFIG_BLUETOOTH_UART)
		.config_func = ns16550_uart_init,
		.irq = CONFIG_UART_CONSOLE_IRQ,
#endif
		.pci_dev.class = PCI_CLASS_COMM_CTLR,
		.pci_dev.bus = CONFIG_UART_PCI_BUS,
		.pci_dev.dev = CONFIG_UART_PCI_DEV,
		.pci_dev.vendor_id = CONFIG_UART_PCI_VENDOR_ID,
		.pci_dev.device_id = CONFIG_UART_PCI_DEVICE_ID,
		.pci_dev.function = CONFIG_UART_PORT_1_FUNCTION,
		.pci_dev.bar = CONFIG_UART_PCI_BAR,
	},
	/* Add pre-configured ports after this. */
};

/**< Device data */
static struct uart_ns16550_dev_data_t ns16550_uart_dev_data[2];

/* UART 0 */
DECLARE_DEVICE_INIT_CONFIG(ns16550_uart0,
			   CONFIG_UART_PORT_0_NAME,
			   &uart_platform_init,
			   &ns16550_uart_dev_cfg[0]);

#if (defined(CONFIG_EARLY_CONSOLE) && \
		defined(CONFIG_UART_CONSOLE) && \
		(CONFIG_UART_CONSOLE_INDEX == 0))
SYS_DEFINE_DEVICE(ns16550_uart0, &ns16550_uart_dev_data[0], PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#else
SYS_DEFINE_DEVICE(ns16550_uart0, &ns16550_uart_dev_data[0], SECONDARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif /* CONFIG_EARLY_CONSOLE */


/* UART 1 */
DECLARE_DEVICE_INIT_CONFIG(ns16550_uart1,
			   CONFIG_UART_PORT_1_NAME,
			   &uart_platform_init,
			   &ns16550_uart_dev_cfg[1]);

#if (defined(CONFIG_EARLY_CONSOLE) && \
		defined(CONFIG_UART_CONSOLE) && \
		(CONFIG_UART_CONSOLE_INDEX == 1))
SYS_DEFINE_DEVICE(ns16550_uart1, &ns16550_uart_dev_data[1], PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#else
SYS_DEFINE_DEVICE(ns16550_uart1, &ns16550_uart_dev_data[1], SECONDARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif /* CONFIG_EARLY_CONSOLE */


/**< UART Devices */
struct device * const uart_devs[] = {
	&__initconfig_ns16550_uart0,
	&__initconfig_ns16550_uart1,
};

#endif

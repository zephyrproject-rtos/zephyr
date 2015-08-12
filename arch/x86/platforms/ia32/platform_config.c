/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file Contains configuration for ia32 platforms.
 */

#include <stdio.h>
#include <stdint.h>
#include <device.h>
#include <init.h>

#include "board.h"

#ifdef CONFIG_NS16550
#include <drivers/uart.h>
#include <bluetooth/uart.h>
#include <console/uart_console.h>
#include <serial/ns16550.h>


#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)

/**
 * @brief Initialize NS16550 serial port as console
 *
 * Initialize the UART port for console I/O.
 *
 * @param dev The UART device struct
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static int ns16550_uart_console_init(struct device *dev)
{
#if defined(CONFIG_UART_CONSOLE_INDEX)
	struct uart_init_info info = {
		.baud_rate = CONFIG_UART_BAUDRATE,
		.sys_clk_freq = UART_XTAL_FREQ,
		.int_pri = CONFIG_UART_CONSOLE_INT_PRI
	};

	if (dev == UART_CONSOLE_DEV) {
		uart_init(UART_CONSOLE_DEV, &info);
	}
#endif

#if defined(CONFIG_BLUETOOTH_UART_INDEX)
	if (dev == BT_UART_DEV) {
		bt_uart_init();
	}
#endif

	return DEV_OK;
}

#else

static int ns16550_uart_console_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_OK;
}

#endif


/**< UART device configuration */
static struct uart_device_config_t ns16550_uart_dev_cfg[] = {
	{
		.port = CONFIG_UART_PORT_0_REGS,
		.irq = CONFIG_UART_PORT_0_IRQ,
		.int_pri = CONFIG_UART_PORT_0_IRQ_PRIORITY,

		#if (CONFIG_UART_CONSOLE_INDEX == 0) \
		    || (CONFIG_BLUETOOTH_UART_INDEX == 0)
			.config_func = ns16550_uart_console_init,
		#endif
	},
	{
		.port = CONFIG_UART_PORT_1_REGS,
		.irq = CONFIG_UART_PORT_1_IRQ,
		.int_pri = CONFIG_UART_PORT_1_IRQ_PRIORITY,

		#if (CONFIG_UART_CONSOLE_INDEX == 1) \
		    || (CONFIG_BLUETOOTH_UART_INDEX == 1)
			.config_func = ns16550_uart_console_init,
		#endif
	},
};

/**< UART device data */
static struct uart_ns16550_dev_data_t ns16550_uart_dev_data[2];

/* UART 0 */
DECLARE_DEVICE_INIT_CONFIG(ns16550_uart0,
			   CONFIG_UART_PORT_0_NAME,
			   &uart_platform_init,
			   &ns16550_uart_dev_cfg[0]);

pure_early_init(ns16550_uart0, &ns16550_uart_dev_data[0]);


/* UART 1 */
DECLARE_DEVICE_INIT_CONFIG(ns16550_uart1,
			   CONFIG_UART_PORT_1_NAME,
			   &uart_platform_init,
			   &ns16550_uart_dev_cfg[1]);

pure_early_init(ns16550_uart1, &ns16550_uart_dev_data[1]);


/**< UART Devices */
struct device * const uart_devs[] = {
	&__initconfig_ns16550_uart00,
	&__initconfig_ns16550_uart10,
};

#endif

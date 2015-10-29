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
 * @file Board config file
 */

#include <device.h>
#include <init.h>

#include <nanokernel.h>

#include "board.h"

#ifdef CONFIG_K20_UART
#include <uart.h>
#include <drivers/k20_pcr.h>
#include <drivers/k20_uart.h>
#include <console/uart_console.h>
#include <serial/k20UartDrv.h>


#if defined(CONFIG_UART_CONSOLE)
#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)

/**
 * @brief Initialize K20 serial port as console
 *
 * Initialize the UART port for console I/O.
 *
 * @param dev The UART device struct
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static int k20_uart_console_init(struct device *dev)
{
	uint32_t port;
	uint32_t rxPin;
	uint32_t txPin;
	union K20_PCR pcr = {0}; /* Pin Control Register */

	/* Port/pin ctrl module */
	volatile struct K20_PORT_PCR *port_pcr_p =
		(volatile struct K20_PORT_PCR *)PERIPH_ADDR_BASE_PCR;

	struct uart_init_info info = {
		.baud_rate = CONFIG_UART_CONSOLE_BAUDRATE,
		.sys_clk_freq = CONFIG_UART_CONSOLE_CLK_FREQ,
		/* Only supported in polling mode, but init all info fields */
		.irq_pri = CONFIG_UART_CONSOLE_INT_PRI
	};

	/* UART0 Rx and Tx pin assignments */
	port = CONFIG_UART_CONSOLE_PORT;
	rxPin = CONFIG_UART_CONSOLE_PORT_RX_PIN;
	txPin = CONFIG_UART_CONSOLE_PORT_TX_PIN;

	/* Enable the UART Rx and Tx Pins */
	pcr.field.mux = CONFIG_UART_CONSOLE_PORT_MUX_FUNC;

	port_pcr_p->port[port].pcr[rxPin] = pcr;
	port_pcr_p->port[port].pcr[txPin] = pcr;

	uart_init(UART_CONSOLE_DEV, &info);

	return DEV_OK;
}

#else

static int k20_uart_console_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_OK;
}

#endif
#endif /* CONFIG_UART_CONSOLE */

/**< K20 UART configuration for individual ports. */
static struct uart_device_config k20_uart_dev_cfg[] = {
	{
		.base = (uint8_t *)CONFIG_UART_PORT_0_REGS,
		.irq = CONFIG_UART_PORT_0_IRQ,

		.port_init = k20_uart_port_init,

		#if (defined(CONFIG_UART_CONSOLE) \
		     && (CONFIG_UART_CONSOLE_INDEX == 0))
			.config_func = k20_uart_console_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_1_REGS,
		.irq = CONFIG_UART_PORT_1_IRQ,

		.port_init = k20_uart_port_init,

		#if (defined(CONFIG_UART_CONSOLE) \
		     && (CONFIG_UART_CONSOLE_INDEX == 1))
			.config_func = k20_uart_console_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_2_REGS,
		.irq = CONFIG_UART_PORT_2_IRQ,

		.port_init = k20_uart_port_init,

		#if (defined(CONFIG_UART_CONSOLE) \
		     && (CONFIG_UART_CONSOLE_INDEX == 2))
			.config_func = k20_uart_console_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_3_REGS,
		.irq = CONFIG_UART_PORT_3_IRQ,

		.port_init = k20_uart_port_init,

		#if (defined(CONFIG_UART_CONSOLE) \
		     && (CONFIG_UART_CONSOLE_INDEX == 3))
			.config_func = k20_uart_console_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_4_REGS,
		.irq = CONFIG_UART_PORT_4_IRQ,

		.port_init = k20_uart_port_init,

		#if (defined(CONFIG_UART_CONSOLE) \
		     && (CONFIG_UART_CONSOLE_INDEX == 4))
			.config_func = k20_uart_console_init,
		#endif
	},
};

/**< Custom device data for each UART port. */
static struct uart_k20_dev_data k20_uart_dev_data[] = {
	{
		.seq_port_num = 0,
	},
	{
		.seq_port_num = 1,
	},
	{
		.seq_port_num = 2,
	},
	{
		.seq_port_num = 3,
	},
	{
		.seq_port_num = 4,
	},
};

/* UART 0 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart0,
			   CONFIG_UART_PORT_0_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[0]);

pre_kernel_late_init(k20_uart0, &k20_uart_dev_data[0]);


/* UART 1 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart1,
			   CONFIG_UART_PORT_1_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[1]);

pre_kernel_late_init(k20_uart1, &k20_uart_dev_data[1]);


/* UART 2 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart2,
			   CONFIG_UART_PORT_2_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[2]);

pre_kernel_late_init(k20_uart2, &k20_uart_dev_data[2]);


/* UART 3 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart3,
			   CONFIG_UART_PORT_3_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[3]);

pre_kernel_late_init(k20_uart3, &k20_uart_dev_data[3]);


/* UART 4 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart4,
			   CONFIG_UART_PORT_4_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[4]);

pre_kernel_late_init(k20_uart4, &k20_uart_dev_data[4]);


/**< UART Devices */
struct device * const uart_devs[] = {
	&__initconfig_k20_uart0,
	&__initconfig_k20_uart1,
	&__initconfig_k20_uart2,
	&__initconfig_k20_uart3,
	&__initconfig_k20_uart4,
};

#endif /* CONFIG_K20_UART */

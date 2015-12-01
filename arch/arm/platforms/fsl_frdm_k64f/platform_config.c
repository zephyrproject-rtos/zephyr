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

#ifdef CONFIG_UART_K20
#include <uart.h>
#include <drivers/k20_pcr.h>
#include <console/uart_console.h>

#include <serial/uart_k20_priv.h>


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
static int uart_k20_console_init(struct device *dev)
{
	uint32_t port;
	uint32_t rxPin;
	uint32_t txPin;
	union K20_PCR pcr = {0}; /* Pin Control Register */

	/* Port/pin ctrl module */
	volatile struct K20_PORT_PCR *port_pcr_p =
		(volatile struct K20_PORT_PCR *)PERIPH_ADDR_BASE_PCR;

	/* UART0 Rx and Tx pin assignments */
	port = CONFIG_UART_CONSOLE_PORT;
	rxPin = CONFIG_UART_CONSOLE_PORT_RX_PIN;
	txPin = CONFIG_UART_CONSOLE_PORT_TX_PIN;

	/* Enable the UART Rx and Tx Pins */
	pcr.field.mux = CONFIG_UART_CONSOLE_PORT_MUX_FUNC;

	port_pcr_p->port[port].pcr[rxPin] = pcr;
	port_pcr_p->port[port].pcr[txPin] = pcr;

	return DEV_OK;
}

DECLARE_DEVICE_INIT_CONFIG(_uart_k20_console, "", uart_k20_console_init, NULL);
SYS_DEFINE_DEVICE(_uart_k20_console, NULL, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
#endif /* CONFIG_UART_CONSOLE */

#endif /* CONFIG_UART_K20 */

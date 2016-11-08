/*
 * Copyright (c) 2015 Intel Corporation.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
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

#include "soc.h"
#include <fsl_common.h>

#ifdef CONFIG_UART_K20
#include <uart.h>
#include <console/uart_console.h>
#include <serial/uart_k20_priv.h>
#endif /* CONFIG_UART_K20 */

/*
 * UART configuration
 */

#ifdef CONFIG_UART_K20

#if defined(CONFIG_UART_CONSOLE) && \
	(defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE))

static PORT_Type *const ports[] = PORT_BASE_PTRS;

/**
 * @brief Initialize K20 serial port as console
 *
 * Initialize the UART port for console I/O.
 *
 * @param dev The UART device struct
 *
 * @return 0 if successful, otherwise failed.
 */
static ALWAYS_INLINE int uart_k20_console_init(void)
{
	PORT_Type *port;
	uint32_t rxPin;
	uint32_t txPin;

	/* Port/pin ctrl module */
	port = ports[CONFIG_UART_CONSOLE_PORT];

	/* UART0 Rx and Tx pin assignments */
	rxPin = CONFIG_UART_CONSOLE_PORT_RX_PIN;
	txPin = CONFIG_UART_CONSOLE_PORT_TX_PIN;

	/* Enable the UART Rx and Tx Pins */
	port->PCR[rxPin] = PORT_PCR_MUX(CONFIG_UART_CONSOLE_PORT_MUX_FUNC);
	port->PCR[txPin] = PORT_PCR_MUX(CONFIG_UART_CONSOLE_PORT_MUX_FUNC);

	return 0;
}

#else
#define uart_k20_console_init(...)
#endif /* CONFIG_UART_CONSOLE && (CONFIG_PRINTK || CONFIG_STDOUT_CONSOLE) */

static int uart_k20_init(struct device *dev)
{
	uint32_t scgc4;

	ARG_UNUSED(dev);

	/* Although it is possible to modify the bits through
	 * *sim directly, the following code saves about 20 bytes
	 * of ROM space, compared to direct modification.
	 */
	scgc4 = SIM->SCGC4;

#ifdef CONFIG_UART_K20_PORT_0
	scgc4 |= SIM_SCGC4_UART0(1);
#endif

#ifdef CONFIG_UART_K20_PORT_1
	scgc4 |= SIM_SCGC4_UART1(1);
#endif

#ifdef CONFIG_UART_K20_PORT_2
	scgc4 |= SIM_SCGC4_UART2(1);
#endif

#ifdef CONFIG_UART_K20_PORT_3
	scgc4 |= SIM_SCGC4_UART3(1);
#endif

	SIM->SCGC4 = scgc4;

#ifdef CONFIG_UART_K20_PORT_4
	SIM->SCGC1 |= SIM_SCGC1_UART4(1);
#endif

	/* Initialize UART port for console if needed */
	uart_k20_console_init();

	return 0;
}

DEVICE_INIT(_uart_k20_init, "", uart_k20_init,
				NULL, NULL,
				PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_UART_K20 */

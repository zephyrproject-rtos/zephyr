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
 * @file Board config file
 */

#include <device.h>
#include <init.h>

#include <nanokernel.h>

#include "board.h"

#ifdef CONFIG_K20_UART
#include <drivers/uart.h>
#include <drivers/k20_pcr.h>
#include <drivers/k20_uart.h>
#include <console/uart_console.h>


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
	K20_PCR_t pcr = {0}; /* Pin Control Register */

	/* Port/pin ctrl module */
	K20_PORT_PCR_t *port_pcr_p = (K20_PORT_PCR_t *)PERIPH_ADDR_BASE_PCR;

	struct uart_init_info info = {
		.baud_rate = CONFIG_UART_CONSOLE_BAUDRATE,
		.sys_clk_freq = CONFIG_UART_CONSOLE_CLK_FREQ,
		/* Only supported in polling mode, but init all info fields */
		.int_pri = CONFIG_UART_CONSOLE_INT_PRI
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

/**< K20 UART configuration for individual ports. */
static struct uart_device_config_t k20_uart_dev_cfg[] = {
	{
		.base = (uint8_t *)CONFIG_UART_PORT_0_REGS,
		.irq = CONFIG_UART_PORT_0_IRQ,

		#if (CONFIG_UART_CONSOLE_INDEX == 0)
			.config_func = k20_uart_console_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_1_REGS,
		.irq = CONFIG_UART_PORT_1_IRQ,

		#if (CONFIG_UART_CONSOLE_INDEX == 1)
			.config_func = k20_uart_console_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_2_REGS,
		.irq = CONFIG_UART_PORT_2_IRQ,

		#if (CONFIG_UART_CONSOLE_INDEX == 2)
			.config_func = k20_uart_console_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_3_REGS,
		.irq = CONFIG_UART_PORT_3_IRQ,

		#if (CONFIG_UART_CONSOLE_INDEX == 3)
			.config_func = k20_uart_console_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_4_REGS,
		.irq = CONFIG_UART_PORT_4_IRQ,

		#if (CONFIG_UART_CONSOLE_INDEX == 4)
			.config_func = k20_uart_console_init,
		#endif
	},
};

/**< Custom device data for each UART port. */
static struct uart_k20_dev_data_t k20_uart_dev_data[] = {
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

pure_early_init(k20_uart0, &k20_uart_dev_data[0]);


/* UART 1 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart1,
			   CONFIG_UART_PORT_1_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[1]);

pure_early_init(k20_uart1, &k20_uart_dev_data[1]);


/* UART 2 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart2,
			   CONFIG_UART_PORT_2_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[2]);

pure_early_init(k20_uart2, &k20_uart_dev_data[2]);


/* UART 3 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart3,
			   CONFIG_UART_PORT_3_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[3]);

pure_early_init(k20_uart3, &k20_uart_dev_data[3]);


/* UART 4 */
DECLARE_DEVICE_INIT_CONFIG(k20_uart4,
			   CONFIG_UART_PORT_4_NAME,
			   &uart_platform_init,
			   &k20_uart_dev_cfg[4]);

pure_early_init(k20_uart4, &k20_uart_dev_data[4]);


/**< UART Devices */
struct device * const uart_devs[] = {
	&__initconfig_k20_uart00,
	&__initconfig_k20_uart10,
	&__initconfig_k20_uart20,
	&__initconfig_k20_uart30,
	&__initconfig_k20_uart40,
};

#endif /* CONFIG_K20_UART */

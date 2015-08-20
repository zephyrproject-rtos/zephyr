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

#ifdef CONFIG_STELLARIS_UART
#include <drivers/uart.h>
#include <console/uart_console.h>
#include <serial/stellarisUartDrv.h>

#ifdef CONFIG_BLUETOOTH_UART
#include <bluetooth/uart.h>
#endif

#define RCGC1 (*((volatile uint32_t *)0x400FE104))

#define RCGC1_UART0_EN 0x00000001
#define RCGC1_UART1_EN 0x00000002
#define RCGC1_UART2_EN 0x00000004


#if defined(CONFIG_UART_CONSOLE)
#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)

/**
 * @brief Initialize Stellaris serial port as console
 *
 * Initialize the UART port for console I/O.
 *
 * @param dev The UART device struct
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static int stellaris_uart_console_init(struct device *dev)
{
	struct uart_init_info info = {
		.sys_clk_freq = SYSCLK_DEFAULT_IOSC_HZ,
		.baud_rate = CONFIG_UART_CONSOLE_BAUDRATE,
		/* Only supported in polling mode, but init all info fields */
		.irq_pri = CONFIG_UART_CONSOLE_INT_PRI,
	};

	uart_init(UART_CONSOLE_DEV, &info);

	return DEV_OK;
}

#else

int stellaris_uart_console_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_OK;
}

#endif /* CONFIG_PRINTK || CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_UART_CONSOLE */


#if (defined(CONFIG_UART_CONSOLE) && (CONFIG_UART_CONSOLE_INDEX == 0)) \
    || (CONFIG_BLUETOOTH_UART_INDEX == 0)

static int stellaris_uart0_init(struct device *dev)
{
#if (defined(CONFIG_UART_CONSOLE) && (CONFIG_UART_CONSOLE_INDEX == 0))
	RCGC1 |= RCGC1_UART0_EN;
	return stellaris_uart_console_init(dev);
#elif (CONFIG_BLUETOOTH_UART_INDEX == 0)
	RCGC1 |= RCGC1_UART0_EN;
	bt_uart_init();
	return DEV_OK;
#endif
}

#endif /* CONFIG_UART_CONSOLE_INDEX == 0 || CONFIG_BLUETOOTH_UART_INDEX == 0 */


#if (defined(CONFIG_UART_CONSOLE) && (CONFIG_UART_CONSOLE_INDEX == 1)) \
    || (CONFIG_BLUETOOTH_UART_INDEX == 1)

static int stellaris_uart1_init(struct device *dev)
{
#if (defined(CONFIG_UART_CONSOLE) && (CONFIG_UART_CONSOLE_INDEX == 1))
	RCGC1 |= RCGC1_UART1_EN;
	return stellaris_uart_console_init(dev);
#elif (CONFIG_BLUETOOTH_UART_INDEX == 1)
	RCGC1 |= RCGC1_UART1_EN;
	bt_uart_init();
	return DEV_OK;
#endif
}

#endif /* CONFIG_UART_CONSOLE_INDEX == 0 || CONFIG_BLUETOOTH_UART_INDEX == 0 */


#if (defined(CONFIG_UART_CONSOLE) && (CONFIG_UART_CONSOLE_INDEX == 2)) \
    || (CONFIG_BLUETOOTH_UART_INDEX == 2)

static int stellaris_uart2_init(struct device *dev)
{
#if (defined(CONFIG_UART_CONSOLE) && (CONFIG_UART_CONSOLE_INDEX == 2))
	RCGC1 |= RCGC1_UART2_EN;
	return stellaris_uart_console_init(dev);
#elif (CONFIG_BLUETOOTH_UART_INDEX == 2)
	RCGC1 |= RCGC1_UART2_EN;
	bt_uart_init();
	return DEV_OK;
#endif
}

#endif /* CONFIG_UART_CONSOLE_INDEX == 2 || CONFIG_BLUETOOTH_UART_INDEX == 2 */


/**< UART device configurations. */
static struct uart_device_config_t stellaris_uart_dev_cfg[] = {
	{
		.base = (uint8_t *)CONFIG_UART_PORT_0_REGS,
		.irq = CONFIG_UART_PORT_0_IRQ,

		.port_init = stellaris_uart_port_init,

		#if (defined(CONFIG_UART_CONSOLE) \
		     && (CONFIG_UART_CONSOLE_INDEX == 0)) \
		    || (CONFIG_BLUETOOTH_UART_INDEX == 0)
			.config_func = stellaris_uart0_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_1_REGS,
		.irq = CONFIG_UART_PORT_1_IRQ,

		.port_init = stellaris_uart_port_init,

		#if (defined(CONFIG_UART_CONSOLE) \
		     && (CONFIG_UART_CONSOLE_INDEX == 1)) \
		    || (CONFIG_BLUETOOTH_UART_INDEX == 1)
			.config_func = stellaris_uart1_init,
		#endif
	},
	{
		.base = (uint8_t *)CONFIG_UART_PORT_2_REGS,
		.irq = CONFIG_UART_PORT_2_IRQ,

		.port_init = stellaris_uart_port_init,

		#if (defined(CONFIG_UART_CONSOLE) \
		     && (CONFIG_UART_CONSOLE_INDEX == 2)) \
		    || (CONFIG_BLUETOOTH_UART_INDEX == 2)
			.config_func = stellaris_uart2_init,
		#endif
	},
};

/* UART 0 */
DECLARE_DEVICE_INIT_CONFIG(stellaris_uart0,
			   CONFIG_UART_PORT_0_NAME,
			   &uart_platform_init,
			   &stellaris_uart_dev_cfg[0]);

pure_init(stellaris_uart0, NULL);


/* UART 1 */
DECLARE_DEVICE_INIT_CONFIG(stellaris_uart1,
			   CONFIG_UART_PORT_1_NAME,
			   &uart_platform_init,
			   &stellaris_uart_dev_cfg[1]);

pure_init(stellaris_uart1, NULL);


/* UART 2 */
DECLARE_DEVICE_INIT_CONFIG(stellaris_uart2,
			   CONFIG_UART_PORT_2_NAME,
			   &uart_platform_init,
			   &stellaris_uart_dev_cfg[2]);

pure_init(stellaris_uart2, NULL);


/**< UART Devices */
struct device * const uart_devs[] = {
	&__initconfig_stellaris_uart01,
	&__initconfig_stellaris_uart11,
	&__initconfig_stellaris_uart21,
};

#endif /* CONFIG_STELLARIS_UART */

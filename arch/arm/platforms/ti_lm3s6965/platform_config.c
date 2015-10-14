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

pre_kernel_late_init(stellaris_uart0, NULL);


/* UART 1 */
DECLARE_DEVICE_INIT_CONFIG(stellaris_uart1,
			   CONFIG_UART_PORT_1_NAME,
			   &uart_platform_init,
			   &stellaris_uart_dev_cfg[1]);

pre_kernel_late_init(stellaris_uart1, NULL);


/* UART 2 */
DECLARE_DEVICE_INIT_CONFIG(stellaris_uart2,
			   CONFIG_UART_PORT_2_NAME,
			   &uart_platform_init,
			   &stellaris_uart_dev_cfg[2]);

pre_kernel_late_init(stellaris_uart2, NULL);


/**< UART Devices */
struct device * const uart_devs[] = {
	&__initconfig_stellaris_uart0,
	&__initconfig_stellaris_uart1,
	&__initconfig_stellaris_uart2,
};

#endif /* CONFIG_STELLARIS_UART */

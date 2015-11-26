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

#include <stdio.h>
#include <stdint.h>
#include <device.h>
#include <init.h>

#include "board.h"
#include <nanokernel.h>
#include <arch/cpu.h>

#ifdef CONFIG_NS16550
#include <uart.h>
#include <console/uart_console.h>
#include <serial/ns16550.h>

#if defined(CONFIG_UART_CONSOLE)
#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)

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

	uart_init(UART_CONSOLE_DEV, &info);

	return DEV_OK;
}

#else

static int ns16550_uart_console_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_OK;
}

#endif
#endif


/* UART 1 */
static struct uart_device_config ns16550_uart1_dev_cfg = {
	.port = CONFIG_UART_PORT_1_REGS,
	.irq = CONFIG_UART_PORT_1_IRQ,
	.irq_pri = CONFIG_UART_PORT_1_IRQ_PRIORITY,

	.port_init = ns16550_uart_port_init,

#if (defined(CONFIG_UART_CONSOLE) && (CONFIG_UART_CONSOLE_INDEX == 0))
	.config_func = ns16550_uart_console_init,
#endif
};

DECLARE_DEVICE_INIT_CONFIG(ns16550_uart1,
			   CONFIG_UART_PORT_1_NAME,
			   &uart_platform_init,
			   &ns16550_uart1_dev_cfg);

static struct uart_ns16550_dev_data_t ns16550_uart1_dev_data;

SYS_DEFINE_DEVICE(ns16550_uart1, &ns16550_uart1_dev_data,
					PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);


/**
 * @brief UART devices
 *
 */
struct device * const uart_devs[] = {
	&__initconfig_ns16550_uart1,
};


#endif
#if CONFIG_IPI_QUARK_SE
#include <ipi.h>
#include <ipi/ipi_quark_se.h>

IRQ_CONNECT_STATIC(quark_se_ipi, QUARK_SE_IPI_INTERRUPT, QUARK_SE_IPI_INTERRUPT_PRI,
		   quark_se_ipi_isr, NULL, 0);

static int x86_quark_se_ipi_init(void)
{
	IRQ_CONFIG(quark_se_ipi, QUARK_SE_IPI_INTERRUPT, 0);
	irq_enable(QUARK_SE_IPI_INTERRUPT);
	return DEV_OK;
}

static struct quark_se_ipi_controller_config_info ipi_controller_config = {
	.controller_init = x86_quark_se_ipi_init
};
DECLARE_DEVICE_INIT_CONFIG(quark_se_ipi, "", quark_se_ipi_controller_initialize,
			   &ipi_controller_config);
SYS_DEFINE_DEVICE(quark_se_ipi, NULL, PRIMARY,
					CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#if defined(CONFIG_IPI_CONSOLE_RECEIVER) && defined(CONFIG_PRINTK)
#include <console/ipi_console.h>

QUARK_SE_IPI_DEFINE(quark_se_ipi4, 4, QUARK_SE_IPI_INBOUND);

#define QUARK_SE_IPI_CONSOLE_LINE_BUF_SIZE	80
#define QUARK_SE_IPI_CONSOLE_RING_BUF_SIZE32	128

static uint32_t ipi_console_ring_buf_data[QUARK_SE_IPI_CONSOLE_RING_BUF_SIZE32];
static char __stack ipi_console_fiber_stack[IPI_CONSOLE_STACK_SIZE];
static char ipi_console_line_buf[QUARK_SE_IPI_CONSOLE_LINE_BUF_SIZE];

struct ipi_console_receiver_config_info quark_se_ipi_receiver_config = {
	.bind_to = "quark_se_ipi4",
	.fiber_stack = ipi_console_fiber_stack,
	.ring_buf_data = ipi_console_ring_buf_data,
	.rb_size32 = QUARK_SE_IPI_CONSOLE_RING_BUF_SIZE32,
	.line_buf = ipi_console_line_buf,
	.lb_size = QUARK_SE_IPI_CONSOLE_LINE_BUF_SIZE,
	.flags = IPI_CONSOLE_PRINTK
};
struct ipi_console_receiver_runtime_data quark_se_ipi_receiver_driver_data;
DECLARE_DEVICE_INIT_CONFIG(ipi_console0, "ipi_console0",
			   ipi_console_receiver_init,
			   &quark_se_ipi_receiver_config);
SYS_DEFINE_DEVICE(ipi_console0, &quark_se_ipi_receiver_driver_data,
					SECONDARY, CONFIG_IPI_CONSOLE_PRIORITY);

#endif /* CONFIG_PRINTK && CONFIG_IPI_CONSOLE_RECEIVER */
#endif /* CONFIG_IPI_QUARK_SE */

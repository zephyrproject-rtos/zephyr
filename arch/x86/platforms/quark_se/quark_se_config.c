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

#if CONFIG_IPI_QUARK_SE
#include <ipi.h>
#include <ipi/ipi_quark_se.h>

IRQ_CONNECT_STATIC(quark_se_ipi, QUARK_SE_IPI_INTERRUPT, QUARK_SE_IPI_INTERRUPT_PRI,
		   quark_se_ipi_isr, NULL, 0);

static int x86_quark_se_ipi_init(void)
{
	IRQ_CONFIG(quark_se_ipi, QUARK_SE_IPI_INTERRUPT);
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

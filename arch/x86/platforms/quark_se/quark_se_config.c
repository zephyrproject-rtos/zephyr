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

#if CONFIG_IPM_QUARK_SE
#include <ipm.h>
#include <ipm/ipm_quark_se.h>

IRQ_CONNECT_STATIC(quark_se_ipm, QUARK_SE_IPM_INTERRUPT, QUARK_SE_IPM_INTERRUPT_PRI,
		   quark_se_ipm_isr, NULL, 0);

static int x86_quark_se_ipm_init(void)
{
	IRQ_CONFIG(quark_se_ipm, QUARK_SE_IPM_INTERRUPT);
	irq_enable(QUARK_SE_IPM_INTERRUPT);
	return DEV_OK;
}

static struct quark_se_ipm_controller_config_info ipm_controller_config = {
	.controller_init = x86_quark_se_ipm_init
};
DECLARE_DEVICE_INIT_CONFIG(quark_se_ipm, "", quark_se_ipm_controller_initialize,
			   &ipm_controller_config);
SYS_DEFINE_DEVICE(quark_se_ipm, NULL, PRIMARY,
					CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#if defined(CONFIG_IPM_CONSOLE_RECEIVER) && defined(CONFIG_PRINTK)
#include <console/ipm_console.h>

QUARK_SE_IPM_DEFINE(quark_se_ipm4, 4, QUARK_SE_IPM_INBOUND);

#define QUARK_SE_IPM_CONSOLE_LINE_BUF_SIZE	80
#define QUARK_SE_IPM_CONSOLE_RING_BUF_SIZE32	128

static uint32_t ipm_console_ring_buf_data[QUARK_SE_IPM_CONSOLE_RING_BUF_SIZE32];
static char __stack ipm_console_fiber_stack[IPM_CONSOLE_STACK_SIZE];
static char ipm_console_line_buf[QUARK_SE_IPM_CONSOLE_LINE_BUF_SIZE];

struct ipm_console_receiver_config_info quark_se_ipm_receiver_config = {
	.bind_to = "quark_se_ipm4",
	.fiber_stack = ipm_console_fiber_stack,
	.ring_buf_data = ipm_console_ring_buf_data,
	.rb_size32 = QUARK_SE_IPM_CONSOLE_RING_BUF_SIZE32,
	.line_buf = ipm_console_line_buf,
	.lb_size = QUARK_SE_IPM_CONSOLE_LINE_BUF_SIZE,
	.flags = IPM_CONSOLE_PRINTK
};
struct ipm_console_receiver_runtime_data quark_se_ipm_receiver_driver_data;
DECLARE_DEVICE_INIT_CONFIG(ipm_console0, "ipm_console0",
			   ipm_console_receiver_init,
			   &quark_se_ipm_receiver_config);
SYS_DEFINE_DEVICE(ipm_console0, &quark_se_ipm_receiver_driver_data,
					SECONDARY, CONFIG_IPM_CONSOLE_PRIORITY);

#endif /* CONFIG_PRINTK && CONFIG_IPM_CONSOLE_RECEIVER */
#endif /* CONFIG_IPM_QUARK_SE */

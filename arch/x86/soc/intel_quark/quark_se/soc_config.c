/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/types.h>
#include <device.h>
#include <init.h>

#include "board.h"
#include <kernel.h>
#include <arch/cpu.h>

#if CONFIG_IPM_QUARK_SE
#include <ipm.h>
#include <ipm/ipm_quark_se.h>

static int x86_quark_se_ipm_init(void)
{
	IRQ_CONNECT(QUARK_SE_IPM_INTERRUPT, CONFIG_QUARK_SE_IPM_IRQ_PRI,
		    quark_se_ipm_isr, NULL, 0);
	irq_enable(QUARK_SE_IPM_INTERRUPT);
	return 0;
}

static struct quark_se_ipm_controller_config_info ipm_controller_config = {
	.controller_init = x86_quark_se_ipm_init
};
DEVICE_AND_API_INIT(quark_se_ipm, "", quark_se_ipm_controller_initialize,
				NULL, &ipm_controller_config,
				PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
				&ipm_quark_se_api_funcs);

#if defined(CONFIG_IPM_CONSOLE_RECEIVER) && defined(CONFIG_PRINTK)
#include <console/ipm_console.h>

QUARK_SE_IPM_DEFINE(quark_se_ipm4, 4, QUARK_SE_IPM_INBOUND);

#define QUARK_SE_IPM_CONSOLE_LINE_BUF_SIZE	80

static u32_t ipm_console_ring_buf_data[CONFIG_QUARK_SE_IPM_CONSOLE_RING_BUF_SIZE32];
static K_THREAD_STACK_DEFINE(ipm_console_thread_stack, CONFIG_IPM_CONSOLE_STACK_SIZE);
static char ipm_console_line_buf[QUARK_SE_IPM_CONSOLE_LINE_BUF_SIZE];

static struct ipm_console_receiver_config_info quark_se_ipm_receiver_config = {
	.bind_to = "quark_se_ipm4",
	.thread_stack = ipm_console_thread_stack,
	.ring_buf_data = ipm_console_ring_buf_data,
	.rb_size32 = CONFIG_QUARK_SE_IPM_CONSOLE_RING_BUF_SIZE32,
	.line_buf = ipm_console_line_buf,
	.lb_size = QUARK_SE_IPM_CONSOLE_LINE_BUF_SIZE,
	.flags = IPM_CONSOLE_PRINTK
};
struct ipm_console_receiver_runtime_data quark_se_ipm_receiver_driver_data;
DEVICE_INIT(ipm_console0, "ipm_console0", ipm_console_receiver_init,
				&quark_se_ipm_receiver_driver_data, &quark_se_ipm_receiver_config,
				POST_KERNEL, CONFIG_IPM_CONSOLE_INIT_PRIORITY);

#endif /* CONFIG_PRINTK && CONFIG_IPM_CONSOLE_RECEIVER */
#endif /* CONFIG_IPM_QUARK_SE */

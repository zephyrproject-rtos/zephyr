/* rtt_console.c - Console messages to a RAM buffer that is then read by
 * the Segger J-Link debugger
 */

/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel.h>
#include <misc/printk.h>
#include <device.h>
#include <init.h>
#include <rtt/SEGGER_RTT.h>

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

static int rtt_console_out(int character)
{
	unsigned int key;
	char c = (char)character;

	key = irq_lock();
	SEGGER_RTT_WriteNoLock(0, &c, 1);
	irq_unlock(key);

	return character;
}

static int rtt_console_init(struct device *d)
{
	ARG_UNUSED(d);

	SEGGER_RTT_Init();

	__printk_hook_install(rtt_console_out);
	__stdout_hook_install(rtt_console_out);

	return 0;
}

SYS_INIT(rtt_console_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

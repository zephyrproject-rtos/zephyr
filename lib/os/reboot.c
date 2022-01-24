/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/timer/system_timer.h>
#include <sys/reboot.h>
#include <kernel.h>
#include <sys/printk.h>

extern void sys_arch_reboot(int type);

FUNC_NORETURN void sys_reboot(int type)
{
	(void)irq_lock();
	sys_clock_disable();

	sys_arch_reboot(type);

	/* should never get here */
	printk("Failed to reboot: spinning endlessly...\n");
	for (;;) {
		k_cpu_idle();
	}
}

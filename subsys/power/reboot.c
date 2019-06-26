/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file common target reboot functionality
 *
 * @details See misc/Kconfig and the reboot help for details.
 */

#include <kernel.h>
#include <drivers/timer/system_timer.h>
#include <sys/printk.h>
#include <power/reboot.h>

extern void sys_arch_reboot(int type);
extern void sys_clock_disable(void);

void sys_reboot(int type)
{
	(void)irq_lock();
#ifdef CONFIG_SYS_CLOCK_EXISTS
	sys_clock_disable();
#endif

	sys_arch_reboot(type);

	/* should never get here */
	printk("Failed to reboot: spinning endlessly...\n");
	for (;;) {
		k_cpu_idle();
	}
}

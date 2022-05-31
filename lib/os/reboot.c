/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

extern void sys_arch_reboot(int type);
extern int sys_arch_get_reboot_type(void);

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

int __weak sys_arch_get_reboot_type(void)
{
	return SYS_REBOOT_WARM;
}

int sys_get_reboot_type(void)
{
	return sys_arch_get_reboot_type();
}

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
#include <drivers/system_timer.h>
#include <logging/sys_log.h>
#include <misc/reboot.h>

extern void sys_arch_reboot(int type);

void sys_reboot(int type)
{
	(void)irq_lock();
	sys_clock_disable();

	sys_arch_reboot(type);

	/* should never get here */
	SYS_LOG_ERR("Failed to reboot: spinning endlessly...");
	for (;;) {
		k_cpu_idle();
	}
}

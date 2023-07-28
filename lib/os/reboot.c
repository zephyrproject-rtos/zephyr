/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>

extern void sys_arch_reboot(enum sys_reboot_mode mode);

FUNC_NORETURN void sys_reboot(enum sys_reboot_mode mode)
{
	(void)irq_lock();

	/* Disable caches to ensure all data is flushed */
	sys_cache_data_disable();
	sys_cache_instr_disable();

	if (IS_ENABLED(CONFIG_SYSTEM_TIMER_HAS_DISABLE_SUPPORT)) {
		sys_clock_disable();
	}

	sys_arch_reboot(mode);

	CODE_UNREACHABLE;
}

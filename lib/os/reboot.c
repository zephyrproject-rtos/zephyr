/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

extern void sys_arch_reboot(int type);

FUNC_NORETURN void sys_reboot(int type)
{
	(void)irq_lock();

	/* Disable caches to ensure all data is flushed */
#if defined(CONFIG_ARCH_CACHE)
#if defined(CONFIG_DCACHE)
	sys_cache_data_disable();
#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)
	sys_cache_instr_disable();
#endif /* CONFIG_ICACHE */
#endif /* CONFIG_ARCH_CACHE */

	if (IS_ENABLED(CONFIG_SYSTEM_TIMER_HAS_DISABLE_SUPPORT)) {
		sys_clock_disable();
	}

	sys_arch_reboot(type);

	/* should never get here */
	printk("Failed to reboot: spinning endlessly...\n");
	for (;;) {
		k_cpu_idle();
	}
}

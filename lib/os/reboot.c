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
#include <zephyr/debug/gcov.h>

extern void sys_arch_reboot(int type);

FUNC_NORETURN void sys_reboot(int type)
{
#ifdef CONFIG_COVERAGE_DUMP
	gcov_coverage_dump();
#elif defined(CONFIG_COVERAGE_SEMIHOST)
	gcov_coverage_semihost();
#endif /* CONFIG_COVERAGE_DUMP */

	(void)irq_lock();

#if defined(CONFIG_ZERO_LATENCY_IRQS)
	(void)arch_zli_lock();
#endif /* CONFIG_ZERO_LATENCY_IRQS */

	/* Disable caches to ensure all data is flushed */
	sys_cache_data_disable();
	sys_cache_instr_disable();

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

/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	printk("!!! system halt reason %u\n", reason);
#ifndef CONFIG_LOG
	printk("!!! set 'CONFIG_LOG=y' in project config for detailed information\n");
#endif

	(void)arch_irq_lock();

#ifdef CONFIG_TELINK_SOC_REBOOT_ON_FAULT
#if CONFIG_TELINK_SOC_REBOOT_ON_FAULT_DELAY
	printk("!!! reboot in %u mS\n", CONFIG_TELINK_SOC_REBOOT_ON_FAULT_DELAY);
	uint64_t start_tick  = sys_clock_cycle_get_64();

	while (sys_clock_cycle_get_64() - start_tick <
		((uint64_t)CONFIG_TELINK_SOC_REBOOT_ON_FAULT_DELAY *
		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / 1000) {
	}

#endif /* CONFIG_TELINK_SOC_REBOOT_ON_FAULT_DELAY */
	printk("!!! reboot\n");
	sys_reboot(0);
	printk("!!! reboot failed: spin endlessly\n");
#endif /* CONFIG_TELINK_SOC_REBOOT_ON_FAULT */
	for (;;) {
		/* Spin endlessly */
		__asm volatile("nop");
	}
}

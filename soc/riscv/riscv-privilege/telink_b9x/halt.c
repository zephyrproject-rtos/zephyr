/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stimer.h>


FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	printk("!!! system halt reason %u\n", reason);
#ifndef CONFIG_LOG
	printk("!!! set 'CONFIG_LOG=y' in project config for detailed information\n");
#endif

	(void)arch_irq_lock();

#ifdef CONFIG_TELINK_SOC_REBOOT_ON_FAULT
	printk("!!! reboot\n\n");
#if CONFIG_TELINK_SOC_REBOOT_ON_FAULT_DELAY
	delay_ms(CONFIG_TELINK_SOC_REBOOT_ON_FAULT_DELAY);
#endif /* CONFIG_TELINK_SOC_REBOOT_ON_FAULT_DELAY */
	extern void sys_arch_reboot(int type);
	sys_arch_reboot(0);
#endif /* CONFIG_TELINK_SOC_REBOOT_ON_FAULT */

	for (;;) {
		/* Spin endlessly */
	}
}

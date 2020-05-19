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
#ifdef CONFIG_ARDUINO_LIKE_UPGRADE
#include <soc.h>
#endif

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

#ifdef CONFIG_ARDUINO_LIKE_UPGRADE

void sys_reboot_to_upgrade(int delay_ms)
{
	k_sleep(Z_TIMEOUT_MS(delay_ms));

	(void)irq_lock();
#ifdef CONFIG_SYS_CLOCK_EXISTS
	sys_clock_disable();
#endif

#ifdef CONFIG_SOC_SERIES_SAMD21
	while (!NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) {
		/* Wait for readiness */
	}

	/* Clear all status bits */
	NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;
	/* Erased first word of the application section causes the stock
	 * bootloader to stay in upload mode. SAMD non-volatile memory is
	 * addressed per half-word so raw byte address needs to be divided by
	 * (word size) / (half-word size) = 2
	 */
	NVMCTRL->ADDR.reg  = NVMCTRL_ADDR_ADDR((CONFIG_FLASH_LOAD_OFFSET + 4) /
						2);
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;

	while (!NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY) {
		/* Wait for readiness */
	}
#endif	// CONFIG_SOC_SERIES_SAMD21

	sys_arch_reboot(SYS_REBOOT_COLD);

	/* should never get here */
	printk("Failed to reboot: spinning endlessly...\n");
	for (;;) {
		k_cpu_idle();
	}
}

#endif	// CONFIG_ARDUINO_LIKE_UPGRADE

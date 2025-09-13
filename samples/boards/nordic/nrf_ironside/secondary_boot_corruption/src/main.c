/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <string.h>

int main(void)
{
	printk("=== Hello World from Primary Image ===\n");

	/* Corrupt the protected memory (This is only possible because we
	 * have disabled CONFIG_ARM_MPU).
	 */
	*((volatile uint32_t *)0xe030000) = 0x5EB0;
	*((volatile uint32_t *)0xe030004) = 0x5EB0;
	*((volatile uint32_t *)0xe030008) = 0x5EB0;
	*((volatile uint32_t *)0xe030010) = 0x5EB0;

	printk("Rebooting\n");

	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

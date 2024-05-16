/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/dfu/mcuboot.h>

/* Main entry point */
int main(void)
{
	int err;

	printk("Launching primary slot application on %s\n", CONFIG_BOARD);
	/* Perform a permanent swap of MCUBoot application */
	err = boot_request_upgrade(BOOT_UPGRADE_PERMANENT);
	if (err) {
		printk("Failed to request upgrade: %d", err);
	} else {
		printk("Secondary application ready for swap, rebooting\n");
		sys_reboot(SYS_REBOOT_COLD);
	}
	return 0;
}

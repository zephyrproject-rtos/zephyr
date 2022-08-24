/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/dfu/mcuboot.h>

/* Main entry point */
void main(void)
{
	printk("Launching primary slot application on %s\n", CONFIG_BOARD);
	/* Perform a permanent swap of MCUBoot application */
	boot_request_upgrade(1);
	printk("Secondary application ready for swap, rebooting\n");
	sys_reboot(SYS_REBOOT_COLD);
}

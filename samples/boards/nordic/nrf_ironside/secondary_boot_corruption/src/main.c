/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/linker/linker-defs.h>
#include <string.h>

int main(void)
{
	printk("=== Hello World from Primary Image ===\n");

	__ASSERT((uint32_t)__rom_region_start == 0xe030000,
			 "For nRF54H20 the protectedmem starts at 0xe030000");

	printk("Corrupting the PROTECTEDMEM\n");
	/* (This is only possible because we have disabled CONFIG_ARM_MPU). */

	*((volatile uint32_t *)__rom_region_start + 0) = 0x5EB0;
	*((volatile uint32_t *)__rom_region_start + 4) = 0x5EB0;
	*((volatile uint32_t *)__rom_region_start + 8) = 0x5EB0;
	*((volatile uint32_t *)__rom_region_start + 12) = 0x5EB0;

	printk("Rebooting\n");

	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}

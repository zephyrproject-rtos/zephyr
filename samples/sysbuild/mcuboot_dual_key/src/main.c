/*
 * Copyright (c) 2026 Intercreate, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

int main(void)
{
	printk("Address of sample %p\n", (void *)__rom_region_start);
	printk("Hello mcuboot dual key! %s\n", CONFIG_BOARD);
	return 0;
}

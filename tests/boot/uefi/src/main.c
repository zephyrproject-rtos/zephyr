/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#ifdef CONFIG_ARM64
#include <zephyr/arch/arm64/efi.h>
#endif

int main(void)
{
#ifdef CONFIG_ARM64
	const struct arm64_efi_boot_info *info = arm64_efi_boot_info_get();

	if (info == NULL || info->memmap_size == 0 || info->memmap_desc_size == 0) {
		printk("No EFI boot information\n");
		return 0;
	}
	printk("EFI memory map: %zu entries\n", info->memmap_size / info->memmap_desc_size);
#endif
	printk("Hello World! %s\n", CONFIG_BOARD);
	return 0;
}

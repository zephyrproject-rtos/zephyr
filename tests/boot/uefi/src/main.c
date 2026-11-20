/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#ifdef CONFIG_ARM64
#include <zephyr/arch/arm64/efi.h>
#include <zephyr/kernel/internal/mm.h>

/* EFI_MEMORY_DESCRIPTOR from the UEFI specification */
struct efi_memory_descriptor {
	uint32_t type;
	uint64_t physical_start;
	uint64_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
};

#define EFI_LOADER_DATA 2

/* Check that the memory map lists Zephyr's RAM as allocated by the EFI stub */
static bool efi_memmap_ok(const struct arm64_efi_boot_info *info)
{
	uintptr_t ram = CONFIG_SRAM_BASE_ADDRESS;
	uint8_t *map;

	k_mem_map_phys_bare(&map, info->memmap, info->memmap_size, K_MEM_CACHE_WB);

	for (size_t off = 0; off < info->memmap_size; off += info->memmap_desc_size) {
		const struct efi_memory_descriptor *d = (const void *)(map + off);
		uint64_t end = d->physical_start + d->number_of_pages * 4096U;

		if (ram >= d->physical_start && ram < end) {
			printk("EFI memory map: %zu entries, Zephyr RAM type %u\n",
			       info->memmap_size / info->memmap_desc_size, d->type);
			return d->type == EFI_LOADER_DATA;
		}
	}
	printk("Zephyr RAM not in the EFI memory map\n");
	return false;
}
#endif

int main(void)
{
#ifdef CONFIG_ARM64
	const struct arm64_efi_boot_info *info = arm64_efi_boot_info_get();

	if (info == NULL || info->memmap_size == 0 || info->memmap_desc_size == 0) {
		printk("No EFI boot information\n");
		return 0;
	}
	if (!efi_memmap_ok(info)) {
		return 0;
	}
#endif
	printk("Hello World! %s\n", CONFIG_BOARD);
	return 0;
}

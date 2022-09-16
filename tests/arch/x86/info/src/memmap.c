/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr/zephyr.h>
#include <zephyr/arch/x86/memmap.h>

void memmap(void)
{
	printk("MEMORY MAP: source is ");

	switch (x86_memmap_source) {
	case X86_MEMMAP_SOURCE_DEFAULT:
		printk("default");
		break;
	case X86_MEMMAP_SOURCE_MULTIBOOT_MEM:
		printk("Multiboot basic map");
		break;
	case X86_MEMMAP_SOURCE_MULTIBOOT_MMAP:
		printk("Multiboot extended map");
		break;
	case X86_MEMMAP_SOURCE_MANUAL:
		printk("manual");
		break;
	default:
		printk("unknown");
	}

	printk(", max %d entries\n", CONFIG_X86_MEMMAP_ENTRIES);

	for (int i = 0; i < CONFIG_X86_MEMMAP_ENTRIES; ++i) {
		struct x86_memmap_entry *entry = x86_memmap + i;
		const char *label;

		switch (entry->type) {
		case X86_MEMMAP_ENTRY_UNUSED:
			continue;
		case X86_MEMMAP_ENTRY_RAM:
			label = "RAM";
			break;
		case X86_MEMMAP_ENTRY_ACPI:
			label = "ACPI";
			break;
		case X86_MEMMAP_ENTRY_NVS:
			label = "NVS";
			break;
		case X86_MEMMAP_ENTRY_DEFECTIVE:
			label = "DEFECTIVE";
			break;
		default:
			label = "UNKNOWN";
		}

		printk("\t%016llx -> %016llx %s (%lldK)\n",
			entry->base, entry->base + entry->length - 1,
			label,
			entry->length / 1024);
	}

	printk("\n\tKernel exclusions:\n");

	for (int i = 0; i < x86_nr_memmap_exclusions; ++i) {
		printk("\t%p -> %p (%s)\n",
			x86_memmap_exclusions[i].start,
			x86_memmap_exclusions[i].end,
			x86_memmap_exclusions[i].name);
	}

	printk("\n");
}

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/arch/x86/multiboot.h>
#include <zephyr/arch/x86/memmap.h>

struct multiboot_info multiboot_info;

/*
 * called very early in the boot process to fetch data out of the multiboot
 * info struct. we need to grab the relevant data before any dynamic memory
 * allocation takes place, lest the struct itself or any data it points to
 * be overwritten before we read it.
 */

static inline void clear_memmap(int index)
{
	while (index < CONFIG_X86_MEMMAP_ENTRIES) {
		x86_memmap[index].type = X86_MEMMAP_ENTRY_UNUSED;
		++index;
	}
}

void z_multiboot_init(struct multiboot_info *info_pa)
{
	struct multiboot_info *info;

#if defined(CONFIG_ARCH_MAPS_ALL_RAM) || !defined(CONFIG_X86_MMU)
	/*
	 * Since the struct from bootloader resides in memory
	 * and all memory is mapped, there is no need to
	 * manually map it before accessing.
	 *
	 * Without MMU, all memory are identity-mapped already
	 * so there is no need to map them again.
	 */
	info = info_pa;
#else
	k_mem_map_phys_bare((uint8_t **)&info, POINTER_TO_UINT(info_pa),
			    sizeof(*info_pa), K_MEM_CACHE_NONE);
#endif /* CONFIG_ARCH_MAPS_ALL_RAM */

	if (info == NULL) {
		return;
	}

	memcpy(&multiboot_info, info, sizeof(*info));

#ifdef CONFIG_MULTIBOOT_MEMMAP
	/*
	 * If the extended map (basically, the equivalent of
	 * the BIOS E820 map) is available, then use that.
	 */

	if ((info->flags & MULTIBOOT_INFO_FLAGS_MMAP) &&
	    (x86_memmap_source < X86_MEMMAP_SOURCE_MULTIBOOT_MMAP)) {
		uintptr_t address;
		uintptr_t address_end;
		struct multiboot_mmap *mmap;
		int index = 0;
		uint32_t type;

#if defined(CONFIG_ARCH_MAPS_ALL_RAM) || !defined(CONFIG_X86_MMU)
		address = info->mmap_addr;
#else
		uint8_t *address_va;

		k_mem_map_phys_bare(&address_va, info->mmap_addr, info->mmap_length,
				    K_MEM_CACHE_NONE);

		address = POINTER_TO_UINT(address_va);
#endif /* CONFIG_ARCH_MAPS_ALL_RAM */

		address_end = address + info->mmap_length;

		while ((address < address_end) &&
		       (index < CONFIG_X86_MEMMAP_ENTRIES)) {
			mmap = UINT_TO_POINTER(address);

			x86_memmap[index].base = mmap->base;
			x86_memmap[index].length = mmap->length;

			switch (mmap->type) {
			case MULTIBOOT_MMAP_RAM:
				type = X86_MEMMAP_ENTRY_RAM;
				break;
			case MULTIBOOT_MMAP_ACPI:
				type = X86_MEMMAP_ENTRY_ACPI;
				break;
			case MULTIBOOT_MMAP_NVS:
				type = X86_MEMMAP_ENTRY_NVS;
				break;
			case MULTIBOOT_MMAP_DEFECTIVE:
				type = X86_MEMMAP_ENTRY_DEFECTIVE;
				break;
			default:
				type = X86_MEMMAP_ENTRY_UNKNOWN;
			}

			x86_memmap[index].type = type;
			++index;
			address += mmap->size + sizeof(mmap->size);
		}

		x86_memmap_source = X86_MEMMAP_SOURCE_MULTIBOOT_MMAP;
		clear_memmap(index);
	}

	/* If no extended map is available, fall back to the basic map. */

	if ((info->flags & MULTIBOOT_INFO_FLAGS_MEM) &&
	    (x86_memmap_source < X86_MEMMAP_SOURCE_MULTIBOOT_MEM)) {
		x86_memmap[0].base = 0;
		x86_memmap[0].length = info->mem_lower * 1024ULL;
		x86_memmap[0].type = X86_MEMMAP_ENTRY_RAM;

		if (CONFIG_X86_MEMMAP_ENTRIES > 1) {
			x86_memmap[1].base = 1048576U; /* 1MB */
			x86_memmap[1].length = info->mem_upper * 1024ULL;
			x86_memmap[1].type = X86_MEMMAP_ENTRY_RAM;
			clear_memmap(2);
		}

		x86_memmap_source = X86_MEMMAP_SOURCE_MULTIBOOT_MEM;
	}
#endif /* CONFIG_MULTIBOOT_MEMMAP */
}

/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr/zephyr.h>
#include <zephyr/arch/x86/multiboot.h>

void multiboot(void)
{
#ifndef CONFIG_MULTIBOOT_INFO
	printk("MULTIBOOT: info struct NOT preserved.\n\n");
#else
	/*
	 * If flags == 0, then either we didn't copy the flags from a real
	 * multiboot info struct because the loader didn't provide one, or
	 * (highly unlikely) we DID get a valid struct, but it was empty.
	 */

	if (!multiboot_info.flags) {
		printk("MULTIBOOT: info struct UNAVAILABLE or EMPTY.\n\n");
	} else {
		printk("MULTIBOOT: boot info structure available.\n");
		printk("\tFlags = 0x%08x\n", multiboot_info.flags);
	}

	if (multiboot_info.flags & MULTIBOOT_INFO_FLAGS_MEM) {
		printk("\tBasic memory map: lower = %dK, upper = %dK.\n",
			multiboot_info.mem_lower,
			multiboot_info.mem_upper);
	} else {
		printk("\tNo basic memory map available.\n");
	}

	if (multiboot_info.flags & MULTIBOOT_INFO_FLAGS_MMAP) {
		printk("\tExtended memory map was at 0x%08x (%d bytes).\n",
			multiboot_info.mmap_addr,
			multiboot_info.mmap_length);
	} else {
		printk("\tNo extended memory map available.\n");
	}

	if (multiboot_info.flags & MULTIBOOT_INFO_FLAGS_FB) {
		printk("\tFramebuffer %dbpp %dX%d (pitch %d) @ %08x.\n",
			multiboot_info.fb_bpp,
			multiboot_info.fb_width,
			multiboot_info.fb_height,
			multiboot_info.fb_pitch,
			multiboot_info.fb_addr_lo);
	} else {
		printk("\tFramebuffer data not present.\n");
	}
#endif
	printk("\n");
}

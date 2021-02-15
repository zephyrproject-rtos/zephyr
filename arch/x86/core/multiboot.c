/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <arch/x86/multiboot.h>
#include <arch/x86/memmap.h>

#ifdef CONFIG_MULTIBOOT_INFO

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

void z_multiboot_init(struct multiboot_info *info)
{
	if (info != NULL) {
		memcpy(&multiboot_info, info, sizeof(*info));
	}

#ifdef CONFIG_MULTIBOOT_MEMMAP
	/*
	 * If the extended map (basically, the equivalent of
	 * the BIOS E820 map) is available, then use that.
	 */

	if ((info->flags & MULTIBOOT_INFO_FLAGS_MMAP) &&
	    (x86_memmap_source < X86_MEMMAP_SOURCE_MULTIBOOT_MMAP)) {
		uint32_t address = info->mmap_addr;
		struct multiboot_mmap *mmap;
		int index = 0;
		uint32_t type;

		while ((address < (info->mmap_addr + info->mmap_length)) &&
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

#ifdef CONFIG_MULTIBOOT_FRAMEBUF

#include <display/framebuf.h>

static struct framebuf_dev_data multiboot_framebuf_data = {
	.width = CONFIG_MULTIBOOT_FRAMEBUF_X,
	.height = CONFIG_MULTIBOOT_FRAMEBUF_Y
};

static int multiboot_framebuf_init(const struct device *dev)
{
	struct framebuf_dev_data *data = FRAMEBUF_DATA(dev);
	struct multiboot_info *info = &multiboot_info;

	if ((info->flags & MULTIBOOT_INFO_FLAGS_FB) &&
	    (info->fb_width >= CONFIG_MULTIBOOT_FRAMEBUF_X) &&
	    (info->fb_height >= CONFIG_MULTIBOOT_FRAMEBUF_Y) &&
	    (info->fb_bpp == 32) && (info->fb_addr_hi == 0)) {
		/*
		 * We have a usable multiboot framebuffer - it is 32 bpp
		 * and at least as large as the requested dimensions. Compute
		 * the pitch and adjust the start address center our canvas.
		 */

		uint16_t adj_x;
		uint16_t adj_y;
		uint32_t *buffer;

		adj_x = info->fb_width - CONFIG_MULTIBOOT_FRAMEBUF_X;
		adj_y = info->fb_height - CONFIG_MULTIBOOT_FRAMEBUF_Y;
		data->pitch = (info->fb_pitch / 4) + adj_x;
		adj_x /= 2;
		adj_y /= 2;
		buffer = (uint32_t *) (uintptr_t) info->fb_addr_lo;
		buffer += adj_x;
		buffer += adj_y * data->pitch;
		data->buffer = buffer;
		return 0;
	} else {
		return -ENOTSUP;
	}
}

DEVICE_DEFINE(multiboot_framebuf,
		    "FRAMEBUF",
		    multiboot_framebuf_init,
		    device_pm_control_nop,
		    &multiboot_framebuf_data,
		    NULL,
		    PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &framebuf_display_api);

#endif /* CONFIG_MULTIBOOT_FRAMEBUF */

#endif /* CONFIG_MULTIBOOT_INFO */

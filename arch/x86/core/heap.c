/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <app_memory/app_memdomain.h>
#include <arch/x86/memmap.h>
#include <linker/linker-defs.h>
#include <sys/sem.h>
#include <sys/util.h>

#define USED_RAM_END_ADDR	POINTER_TO_UINT(&_end)

static K_APP_DMEM(z_libc_partition) SYS_SEM_DEFINE(heap_sem, 1, 1);

/* Start of heap area */
static K_APP_BMEM(z_libc_partition) uintptr_t heap_start;

/* End of heap area */
static K_APP_BMEM(z_libc_partition) uintptr_t heap_end;

/* Current heap pointer used as needed for sbrk() */
static K_APP_BMEM(z_libc_partition) intptr_t heap_cur_ptr;

void *sbrk(int count)
{
	void *ret;
	intptr_t ptr;

	/* coverity[CHECKED_RETURN] */
	sys_sem_take(&heap_sem, K_FOREVER);

	ptr = heap_cur_ptr + count;

	if ((ptr >= heap_start) && (ptr < heap_end)) {
		heap_cur_ptr = ptr;
		ret = INT_TO_POINTER(ptr);
	} else {
		ret = INT_TO_POINTER(-1);
	}

	/* coverity[CHECKED_RETURN] */
	sys_sem_give(&heap_sem);

	return ret;
}

static int z_x86_heap_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_MMU
	heap_start = ROUND_UP(USED_RAM_END_ADDR, CONFIG_MMU_PAGE_SIZE);
#else
	heap_start = USED_RAM_END_ADDR;
#endif

	heap_cur_ptr = heap_start;

#ifndef CONFIG_MULTIBOOT_MEMMAP
	/*
	 * Without memory map from multiboot, we don't know where
	 * reserved memory areas are. So, by default, use the whole
	 * remaining SRAM area as heap.
	 */

	uintptr_t heap_size = (KB(CONFIG_SRAM_SIZE) -
			       (heap_start - CONFIG_SRAM_BASE_ADDRESS));

	heap_end = heap_start + heap_size;
#else
	/*
	 * Use multiboot memory map to figure out the range of memory
	 * that can be used for heap.
	 */

	int i;
	uintptr_t mem_start, mem_end;

	/* By default, no heap area  */
	heap_end = heap_start;

	/*
	 * Go through all entry in memory map to figure out where
	 * the heap can be.
	 */
	for (i = 0; i < CONFIG_X86_MEMMAP_ENTRIES; ++i) {
		struct x86_memmap_entry *entry = &x86_memmap[i];

		if (entry->type == X86_MEMMAP_ENTRY_RAM) {
			mem_start = entry->base;
			mem_end = entry->base + entry->length;

			/*
			 * The start of heap is within in this memory
			 * region. Heap can extend to end of this
			 * region.
			 */
			if ((heap_start >= mem_start) &&
			    (heap_start < mem_end)) {
				heap_end = mem_end;
				break;
			}
		}
	}

#endif

	return 0;
}

SYS_INIT(z_x86_heap_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

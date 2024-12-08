/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <mmu.h>
#include <kernel_arch_interface.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/common/semihost.h>

/*
 * semihost.h declares prototypes with longs but (at least on QEMU)
 * returned values are 32-bits only. Let's use an int.
 */
static int semih_fd = -1;

int k_mem_paging_backing_store_location_get(struct k_mem_page_frame *pf,
					    uintptr_t *location,
					    bool page_fault)
{
	if (k_mem_page_frame_is_backed(pf)) {
		return k_mem_paging_backing_store_location_query(
				k_mem_page_frame_to_virt(pf), location);
	} else {
		/* this is a read-only backing store */
		return -ENOMEM;
	}
}

void k_mem_paging_backing_store_location_free(uintptr_t location)
{
}

void k_mem_paging_backing_store_page_out(uintptr_t location)
{
	__ASSERT(true, "not ever supposed to be called");
	k_panic();
}

void k_mem_paging_backing_store_page_in(uintptr_t location)
{
	long size = CONFIG_MMU_PAGE_SIZE;

	if (semihost_seek(semih_fd, (long)location) != 0 ||
	    semihost_read(semih_fd, K_MEM_SCRATCH_PAGE, size) != size) {
		k_panic();
	}
}

void k_mem_paging_backing_store_page_finalize(struct k_mem_page_frame *pf,
					      uintptr_t location)
{
	k_mem_page_frame_set(pf, K_MEM_PAGE_FRAME_BACKED);
}

int k_mem_paging_backing_store_location_query(void *addr, uintptr_t *location)
{
	uintptr_t offset = (uintptr_t)addr - (uintptr_t)lnkr_ondemand_start;
	uintptr_t file_offset = (uintptr_t)lnkr_ondemand_load_start
				- (uintptr_t)__text_region_start + offset;

	__ASSERT(file_offset % CONFIG_MMU_PAGE_SIZE == 0, "file_offset = %#lx", file_offset);
	*location = file_offset;
	return 0;
}

void k_mem_paging_backing_store_init(void)
{
	semih_fd = semihost_open("./zephyr/zephyr.bin", SEMIHOST_OPEN_RB);
	__ASSERT(semih_fd >= 0, "semihost_open() returned %d", semih_fd);
	if (semih_fd < 0) {
		k_panic();
	}
}

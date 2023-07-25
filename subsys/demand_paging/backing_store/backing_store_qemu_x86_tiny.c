/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Backing store on qemu_x86_tiny for testing
 *
 * This uses the "flash" memory area (in DTS) as the backing store
 * for demand paging. The qemu_x86_tiny.ld linker script puts
 * the symbols outside of boot and pinned sections into the flash
 * area, allowing testing of the demand paging mechanism on
 * code and data.
 */

#include <mmu.h>
#include <string.h>
#include <kernel_arch_interface.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>

void *location_to_flash(uintptr_t location)
{
	uintptr_t ptr = location;

	/* Offset from start of virtual address space */
	ptr -= CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_OFFSET;

	/* Translate the offset into address to flash */
	ptr += CONFIG_FLASH_BASE_ADDRESS;

	__ASSERT_NO_MSG(ptr >= CONFIG_FLASH_BASE_ADDRESS);
	__ASSERT_NO_MSG(ptr < (CONFIG_FLASH_BASE_ADDRESS
			       + KB(CONFIG_FLASH_SIZE)
			       - CONFIG_MMU_PAGE_SIZE));

	return UINT_TO_POINTER(ptr);
}

int k_mem_paging_backing_store_location_get(struct z_page_frame *pf,
					    uintptr_t *location,
					    bool page_fault)
{
	/* Simply returns the virtual address */
	*location = POINTER_TO_UINT(pf->addr);

	return 0;
}

void k_mem_paging_backing_store_location_free(uintptr_t location)
{
	/* Nothing to do */
}

void k_mem_paging_backing_store_page_out(uintptr_t location)
{
	(void)memcpy(location_to_flash(location), Z_SCRATCH_PAGE,
		     CONFIG_MMU_PAGE_SIZE);
}

void k_mem_paging_backing_store_page_in(uintptr_t location)
{
	(void)memcpy(Z_SCRATCH_PAGE, location_to_flash(location),
		     CONFIG_MMU_PAGE_SIZE);
}

void k_mem_paging_backing_store_page_finalize(struct z_page_frame *pf,
					      uintptr_t location)
{
	/* Nothing to do */
}

void k_mem_paging_backing_store_init(void)
{
	/* Nothing to do */
}

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Routines for managing virtual address spaces
 */

 #include <stdint.h>
 #include <kernel_arch_interface.h>
 #include <spinlock.h>

#define LOG_LEVEL CONFIG_KERNEL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

/* Spinlock to protect any globals in this file and serialize page table
 * updates in arch code
 */
static struct k_spinlock mm_lock;

size_t k_mem_region_align(uintptr_t *aligned_addr, size_t *aligned_size,
			  uintptr_t phys_addr, size_t size, size_t align)
{
	size_t addr_offset;

	/* The actual mapped region must be page-aligned. Round down the
	 * physical address and pad the region size appropriately
	 */
	*aligned_addr = ROUND_DOWN(phys_addr, align);
	addr_offset = phys_addr - *aligned_addr;
	*aligned_size = ROUND_UP(size + addr_offset, align);

	return addr_offset;
}

void k_mem_map(uint8_t **virt_addr, uintptr_t phys_addr, size_t size,
	       uint32_t flags)
{
	uintptr_t aligned_addr, addr_offset;
	size_t aligned_size;
	int ret;
	k_spinlock_key_t key;
	uint8_t *dest_virt;

	addr_offset = k_mem_region_align(&aligned_addr, &aligned_size,
					 phys_addr, size,
					 CONFIG_MMU_PAGE_SIZE);

	/* Carve out some unused virtual memory from the top of the
	 * address space
	 */
	key = k_spin_lock(&mm_lock);

	/* TODO: For now, do an identity mapping, we haven't implemented
	 * virtual memory yet
	 */
	dest_virt = (uint8_t *)aligned_addr;

	LOG_DBG("arch_mem_map(%p, 0x%lx, %zu, %x) offset %lu\n", dest_virt,
		aligned_addr, aligned_size, flags, addr_offset);
	__ASSERT(dest_virt != NULL, "NULL memory mapping");
	__ASSERT(aligned_size != 0, "0-length mapping at 0x%lx", aligned_addr);
	ret = arch_mem_map(dest_virt, aligned_addr, aligned_size, flags);
	k_spin_unlock(&mm_lock, key);

	if (ret == 0) {
		*virt_addr = dest_virt + addr_offset;
	} else {
		/* This happens if there is an insurmountable problem
		 * with the selected cache modes or access flags
		 * with no safe fallback
		 */

		LOG_ERR("arch_mem_map() to %p returned %d", dest_virt, ret);
		goto fail;
	}
	return;
fail:
	LOG_ERR("memory mapping 0x%lx (size %zu, flags 0x%x) failed",
		phys_addr, size, flags);
	k_panic();
}

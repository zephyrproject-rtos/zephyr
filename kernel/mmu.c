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

/*
 * Overall virtual memory map. System RAM is identity-mapped:
 *
 * +--------------+ <- CONFIG_SRAM_BASE_ADDRESS
 * | Mapping for  |
 * | all RAM      |
 * |              |
 * |              |
 * +--------------+ <- CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE
 * | Available    |    also the mapping limit as mappings grown downward
 * | virtual mem  |
 * |              |
 * |..............| <- mapping_pos (grows downward as more mappings are made)
 * | Mapping      |
 * +--------------+
 * | Mapping      |
 * +--------------+
 * | ...          |
 * +--------------+
 * | Mapping      |
 * +--------------+ <- CONFIG_SRAM_BASE_ADDRESS + CONFIG_KERNEL_VM_SIZE
 *
 * At the moment we just have one area for mappings and they are permanent.
 * This is under heavy development and may change.
 */

 /* Current position for memory mappings in kernel memory.
  * At the moment, all kernel memory mappings are permanent.
  * z_mem_map() mappings start at the end of the address space, and grow
  * downward.
  *
  * TODO: If we ever encounter a board with RAM in high enough memory
  * such that there isn't room in the address space, define mapping_pos
  * and mapping_limit such that we have mappings grow downward from the
  * beginning of system RAM.
  */
static uint8_t *mapping_pos =
		(uint8_t *)((uintptr_t)(CONFIG_SRAM_BASE_ADDRESS +
					CONFIG_KERNEL_VM_SIZE));

/* Lower-limit of virtual address mapping. Immediately below this is the
 * permanent identity mapping for all SRAM.
 */
static uint8_t *mapping_limit =
	(uint8_t *)((uintptr_t)CONFIG_SRAM_BASE_ADDRESS +
		    KB((size_t)CONFIG_SRAM_SIZE));

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

void z_mem_map(uint8_t **virt_addr, uintptr_t phys_addr, size_t size,
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

	key = k_spin_lock(&mm_lock);

	/* Carve out some unused virtual memory from the top of the
	 * address space
	 */
	if ((mapping_pos - aligned_size) < mapping_limit) {
		LOG_ERR("insufficient kernel virtual address space");
		goto fail;
	}
	mapping_pos -= aligned_size;
	dest_virt = mapping_pos;

	LOG_DBG("arch_mem_map(%p, 0x%lx, %zu, %x) offset %lu\n", dest_virt,
		aligned_addr, aligned_size, flags, addr_offset);
	__ASSERT(dest_virt != NULL, "NULL page memory mapping");
	__ASSERT(aligned_size != 0, "0-length mapping at 0x%lx", aligned_addr);
	__ASSERT((uintptr_t)dest_virt <
		 ((uintptr_t)dest_virt + (aligned_size - 1)),
		 "wraparound for virtual address %p (size %zu)",
		 dest_virt, size);
	__ASSERT(aligned_addr < (aligned_addr + (size - 1)),
		 "wraparound for physical address 0x%lx (size %zu)",
		 aligned_addr, size);

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

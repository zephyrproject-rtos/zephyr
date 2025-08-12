/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common Memory Management Driver Code
 *
 * This file provides common implementation of memory management driver
 * functions, for example, sys_mm_drv_map_region() can use
 * sys_mm_drv_map_page() to map page by page for the whole region.
 * This avoids duplicate implementations of same functionality in
 * different drivers. The implementations here are marked as
 * weak functions so they can be overridden by the driver.
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/mm/system_mm.h>

#include "mm_drv_common.h"

struct k_spinlock sys_mm_drv_common_lock;

int sys_mm_drv_map_region_safe(const struct sys_mm_drv_region *virtual_region,
			       void *virt, uintptr_t phys, size_t size,
			       uint32_t flags)
{
	uintptr_t virtual_region_start = POINTER_TO_UINT(virtual_region->addr);
	uintptr_t virtual_region_end = virtual_region_start + virtual_region->size;

	/* check if memory to be mapped is within given virtual region */
	if ((POINTER_TO_UINT(virt) >= virtual_region_start) &&
	    (POINTER_TO_UINT(virt) + size < virtual_region_end)) {
		return sys_mm_drv_map_region(virt, phys, size, flags);
	}

	return -EINVAL;
}

int sys_mm_drv_map_page_safe(const struct sys_mm_drv_region *virtual_region,
			     void *virt, uintptr_t phys, uint32_t flags)
{
	uintptr_t virtual_region_start = POINTER_TO_UINT(virtual_region->addr);
	uintptr_t virtual_region_end = virtual_region_start + virtual_region->size;

	/* check if memory to be mapped is within given virtual region */
	if ((POINTER_TO_UINT(virt) >= virtual_region_start) &&
	    (POINTER_TO_UINT(virt) + CONFIG_MM_DRV_PAGE_SIZE < virtual_region_end)) {
		return sys_mm_drv_map_page(virt, phys, flags);
	}

	return -EINVAL;
}

bool sys_mm_drv_is_addr_array_aligned(uintptr_t *addr, size_t cnt)
{
	size_t idx;
	bool ret = true;

	for (idx = 0; idx < cnt; idx++) {
		if (!sys_mm_drv_is_addr_aligned(addr[idx])) {
			ret = false;
			break;
		}
	}

	return ret;
}

bool sys_mm_drv_is_virt_region_mapped(void *virt, size_t size)
{
	size_t offset;
	bool ret = true;

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		uint8_t *va = (uint8_t *)virt + offset;

		if (sys_mm_drv_page_phys_get(va, NULL) != 0) {
			ret = false;
			break;
		}
	}

	return ret;
}

bool sys_mm_drv_is_virt_region_unmapped(void *virt, size_t size)
{
	size_t offset;
	bool ret = true;

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		uint8_t *va = (uint8_t *)virt + offset;

		if (sys_mm_drv_page_phys_get(va, NULL) != -EFAULT) {
			ret = false;
			break;
		}
	}

	return ret;
}

/**
 * Unmap a memory region with synchronization already locked.
 *
 * @param virt Page-aligned base virtual address to un-map
 * @param size Page-aligned region size
 * @param is_reset True if resetting the mappings
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual address is not mapped
 */
static int unmap_locked(void *virt, size_t size, bool is_reset)
{
	int ret = 0;
	size_t offset;

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		uint8_t *va = (uint8_t *)virt + offset;

		int ret2 = sys_mm_drv_unmap_page(va);

		if (ret2 != 0) {
			if (is_reset) {
				__ASSERT(false, "cannot reset mapping %p\n", va);
			} else {
				__ASSERT(false, "cannot unmap %p\n", va);
			}

			ret = ret2;
		}
	}

	return ret;
}

int sys_mm_drv_simple_map_region(void *virt, uintptr_t phys,
				 size_t size, uint32_t flags)
{
	k_spinlock_key_t key;
	int ret = 0;
	size_t offset;

	CHECKIF(!sys_mm_drv_is_addr_aligned(phys) ||
		!sys_mm_drv_is_virt_addr_aligned(virt) ||
		!sys_mm_drv_is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&sys_mm_drv_common_lock);

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		uint8_t *va = (uint8_t *)virt + offset;
		uintptr_t pa = phys + offset;

		ret = sys_mm_drv_map_page(va, pa, flags);

		if (ret != 0) {
			__ASSERT(false, "cannot map 0x%lx to %p\n", pa, va);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt, offset, true);

			break;
		}
	}

	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	return ret;
}

__weak FUNC_ALIAS(sys_mm_drv_simple_map_region,
		  sys_mm_drv_map_region, int);

int sys_mm_drv_simple_map_array(void *virt, uintptr_t *phys,
				size_t cnt, uint32_t flags)
{
	k_spinlock_key_t key;
	int ret = 0;
	size_t idx, offset;

	CHECKIF(!sys_mm_drv_is_addr_array_aligned(phys, cnt) ||
		!sys_mm_drv_is_virt_addr_aligned(virt)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&sys_mm_drv_common_lock);

	offset = 0;
	idx = 0;
	while (idx < cnt) {
		uint8_t *va = (uint8_t *)virt + offset;

		ret = sys_mm_drv_map_page(va, phys[idx], flags);

		if (ret != 0) {
			__ASSERT(false, "cannot map 0x%lx to %p\n", phys[idx], va);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt, offset, true);

			break;
		}

		offset += CONFIG_MM_DRV_PAGE_SIZE;
		idx++;
	}

	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	return ret;
}

__weak FUNC_ALIAS(sys_mm_drv_simple_map_array, sys_mm_drv_map_array, int);

int sys_mm_drv_simple_unmap_region(void *virt, size_t size)
{
	k_spinlock_key_t key;
	int ret = 0;

	CHECKIF(!sys_mm_drv_is_virt_addr_aligned(virt) ||
		!sys_mm_drv_is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&sys_mm_drv_common_lock);

	ret = unmap_locked(virt, size, false);

	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	return ret;
}

__weak FUNC_ALIAS(sys_mm_drv_simple_unmap_region,
		  sys_mm_drv_unmap_region, int);

int sys_mm_drv_simple_remap_region(void *virt_old, size_t size,
				   void *virt_new)
{
	k_spinlock_key_t key;
	size_t offset;
	int ret = 0;

	CHECKIF(!sys_mm_drv_is_virt_addr_aligned(virt_old) ||
		!sys_mm_drv_is_virt_addr_aligned(virt_new) ||
		!sys_mm_drv_is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	if ((POINTER_TO_UINT(virt_new) >= POINTER_TO_UINT(virt_old)) &&
	    (POINTER_TO_UINT(virt_new) < (POINTER_TO_UINT(virt_old) + size))) {
		ret = -EINVAL; /* overlaps */
		goto out;
	}

	key = k_spin_lock(&sys_mm_drv_common_lock);

	if (!sys_mm_drv_is_virt_region_mapped(virt_old, size) ||
	    !sys_mm_drv_is_virt_region_unmapped(virt_new, size)) {
		ret = -EINVAL;
		goto unlock_out;
	}

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		uint8_t *va_old = (uint8_t *)virt_old + offset;
		uint8_t *va_new = (uint8_t *)virt_new + offset;
		uintptr_t pa;
		uint32_t flags;

		/*
		 * Grab the physical address of the old mapped page
		 * so the new page can map to the same physical address.
		 */
		ret = sys_mm_drv_page_phys_get(va_old, &pa);
		if (ret != 0) {
			__ASSERT(false, "cannot query %p\n", va_old);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt_new, offset, true);

			goto unlock_out;
		}

		/*
		 * Grab the flags of the old mapped page
		 * so the new page can map to the same flags.
		 */
		ret = sys_mm_drv_page_flag_get(va_old, &flags);
		if (ret != 0) {
			__ASSERT(false, "cannot query page %p\n", va_old);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt_new, offset, true);

			goto unlock_out;
		}

		ret = sys_mm_drv_map_page(va_new, pa, flags);
		if (ret != 0) {
			__ASSERT(false, "cannot map 0x%lx to %p\n", pa, va_new);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt_new, offset, true);

			goto unlock_out;
		}
	}

	(void)unmap_locked(virt_old, size, false);

unlock_out:
	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	return ret;
}

__weak FUNC_ALIAS(sys_mm_drv_simple_remap_region,
		  sys_mm_drv_remap_region, int);

int sys_mm_drv_simple_move_region(void *virt_old, size_t size,
				  void *virt_new, uintptr_t phys_new)
{
	k_spinlock_key_t key;
	size_t offset;
	int ret = 0;

	CHECKIF(!sys_mm_drv_is_addr_aligned(phys_new) ||
		!sys_mm_drv_is_virt_addr_aligned(virt_old) ||
		!sys_mm_drv_is_virt_addr_aligned(virt_new) ||
		!sys_mm_drv_is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	if ((POINTER_TO_UINT(virt_new) >= POINTER_TO_UINT(virt_old)) &&
	    (POINTER_TO_UINT(virt_new) < (POINTER_TO_UINT(virt_old) + size))) {
		ret = -EINVAL; /* overlaps */
		goto out;
	}

	key = k_spin_lock(&sys_mm_drv_common_lock);

	if (!sys_mm_drv_is_virt_region_mapped(virt_old, size) ||
	    !sys_mm_drv_is_virt_region_unmapped(virt_new, size)) {
		ret = -EINVAL;
		goto unlock_out;
	}

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		uint8_t *va_old = (uint8_t *)virt_old + offset;
		uint8_t *va_new = (uint8_t *)virt_new + offset;
		uintptr_t pa = phys_new + offset;
		uint32_t flags;

		ret = sys_mm_drv_page_flag_get(va_old, &flags);
		if (ret != 0) {
			__ASSERT(false, "cannot query page %p\n", va_old);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt_new, offset, true);

			goto unlock_out;
		}

		/*
		 * Map the new page with flags of the old mapped page
		 * so they both have the same properties.
		 */
		ret = sys_mm_drv_map_page(va_new, pa, flags);
		if (ret != 0) {
			__ASSERT(false, "cannot map 0x%lx to %p\n", pa, va_new);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt_new, offset, true);

			goto unlock_out;
		}
	}

	/* Once new mappings are in place, copy the content over. */
	(void)memcpy(virt_new, virt_old, size);

	/* Unmap old virtual memory region once the move is done. */
	(void)unmap_locked(virt_old, size, false);

unlock_out:
	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	return ret;
}

__weak FUNC_ALIAS(sys_mm_drv_simple_move_region,
		  sys_mm_drv_move_region, int);

int sys_mm_drv_simple_move_array(void *virt_old, size_t size,
				 void *virt_new,
				 uintptr_t *phys_new, size_t phys_cnt)
{
	k_spinlock_key_t key;
	size_t idx, offset;
	int ret = 0;

	CHECKIF(!sys_mm_drv_is_addr_array_aligned(phys_new, phys_cnt) ||
		!sys_mm_drv_is_virt_addr_aligned(virt_old) ||
		!sys_mm_drv_is_virt_addr_aligned(virt_new) ||
		!sys_mm_drv_is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	if ((POINTER_TO_UINT(virt_new) >= POINTER_TO_UINT(virt_old)) &&
	    (POINTER_TO_UINT(virt_new) < (POINTER_TO_UINT(virt_old) + size))) {
		ret = -EINVAL; /* overlaps */
		goto out;
	}

	key = k_spin_lock(&sys_mm_drv_common_lock);

	if (!sys_mm_drv_is_virt_region_mapped(virt_old, size) ||
	    !sys_mm_drv_is_virt_region_unmapped(virt_new, size)) {
		ret = -EINVAL;
		goto unlock_out;
	}

	offset = 0;
	idx = 0;
	while (idx < phys_cnt) {
		uint8_t *va_old = (uint8_t *)virt_old + offset;
		uint8_t *va_new = (uint8_t *)virt_new + offset;
		uint32_t flags;

		ret = sys_mm_drv_page_flag_get(va_old, &flags);
		if (ret != 0) {
			__ASSERT(false, "cannot query page %p\n", va_old);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt_new, offset, true);

			goto unlock_out;
		}

		/*
		 * Only map the new page when we can retrieve
		 * flags of the old mapped page as We don't
		 * want to map with unknown random flags.
		 */
		ret = sys_mm_drv_map_page(va_new, phys_new[idx], flags);
		if (ret != 0) {
			__ASSERT(false, "cannot map 0x%lx to %p\n",
				 phys_new[idx], va_new);

			/*
			 * Reset the already mapped virtual addresses.
			 * Note the offset is at the current failed address
			 * which will not be included during unmapping.
			 */
			(void)unmap_locked(virt_new, offset, true);

			goto unlock_out;
		}

		offset += CONFIG_MM_DRV_PAGE_SIZE;
		idx++;
	}

	/* Once new mappings are in place, copy the content over. */
	(void)memcpy(virt_new, virt_old, size);

	/* Unmap old virtual memory region once the move is done. */
	(void)unmap_locked(virt_old, size, false);

unlock_out:
	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	return ret;
}

__weak FUNC_ALIAS(sys_mm_drv_simple_move_array,
		  sys_mm_drv_move_array, int);

int sys_mm_drv_simple_update_region_flags(void *virt, size_t size, uint32_t flags)
{
	k_spinlock_key_t key;
	int ret = 0;
	size_t offset;

	CHECKIF(!sys_mm_drv_is_virt_addr_aligned(virt) ||
		!sys_mm_drv_is_size_aligned(size)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&sys_mm_drv_common_lock);

	for (offset = 0; offset < size; offset += CONFIG_MM_DRV_PAGE_SIZE) {
		uint8_t *va = (uint8_t *)virt + offset;

		int ret2 = sys_mm_drv_update_page_flags(va, flags);

		if (ret2 != 0) {
			__ASSERT(false, "cannot update flags %p\n", va);

			ret = ret2;
		}
	}

	k_spin_unlock(&sys_mm_drv_common_lock, key);

out:
	return ret;
}

__weak FUNC_ALIAS(sys_mm_drv_simple_update_region_flags,
		  sys_mm_drv_update_region_flags, int);

const struct sys_mm_drv_region *sys_mm_drv_simple_query_memory_regions(void)
{
	const static struct sys_mm_drv_region empty[] = {
		{ }
	};

	return empty;
}

__weak FUNC_ALIAS(sys_mm_drv_simple_query_memory_regions,
		  sys_mm_drv_query_memory_regions,
		  const struct sys_mm_drv_region *);

void sys_mm_drv_simple_query_memory_regions_free(const struct sys_mm_drv_region *regions)
{
	ARG_UNUSED(regions);
}

__weak FUNC_ALIAS(sys_mm_drv_simple_query_memory_regions_free,
		  sys_mm_drv_query_memory_regions_free, void);

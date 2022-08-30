/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SYSTEM_MM_DRV_COMMON_H_
#define ZEPHYR_DRIVERS_SYSTEM_MM_DRV_COMMON_H_

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

#include <zephyr/drivers/mm/system_mm.h>

extern struct k_spinlock sys_mm_drv_common_lock;

/**
 * @brief Get the flags of mapped virtual address.
 *
 * The function queries the translation tables to find the flags of
 * a mapped virtual address. This is used internally for remapping.
 *
 * Behavior when providing unaligned address is undefined, this
 * is assumed to be page aligned.
 *
 * @param      virt  Page-aligned virtual address
 * @param[out] flags flags of mapped virtual address
 *
 * @retval 0 if mapping is found and valid
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual address is not mapped
 */
int sys_mm_drv_page_flag_get(void *virt, uint32_t *flags);

/**
 * @brief Test if address is page-aligned
 *
 * @param addr address to be tested
 *
 * @retval true if page-aligned
 * @retval false if not page-aligned
 */
static inline bool sys_mm_drv_is_addr_aligned(uintptr_t addr)
{
	return ((addr & (CONFIG_MM_DRV_PAGE_SIZE - 1)) == 0U);
}

/**
 * @brief Test if address is page-aligned
 *
 * @param addr address to be tested
 *
 * @retval true if page-aligned
 * @retval false if not page-aligned
 */
static inline bool sys_mm_drv_is_virt_addr_aligned(void *virt)
{
	return sys_mm_drv_is_addr_aligned(POINTER_TO_UINT(virt));
}

/**
 * @brief Test if size is page-aligned
 *
 * @param addr size to be tested
 *
 * @retval true if page-aligned
 * @retval false if not page-aligned
 */
static inline bool sys_mm_drv_is_size_aligned(size_t size)
{
	if ((size & (CONFIG_MM_DRV_PAGE_SIZE - 1)) == 0U) {
		return true;
	} else {
		return false;
	}
}

/**
 * @brief Test if all physical addresses in array are page-aligned
 *
 * @param addr Array of physical addresses
 * @param cnt Number of elements in the array
 *
 * @retval true if all are page-aligned
 * @retval false if at least one is not page-aligned
 */
bool sys_mm_drv_is_addr_array_aligned(uintptr_t *addr, size_t cnt);

/**
 * @brief Test if the virtual memory region is mapped
 *
 * @param virt Page-aligned base virtual address
 * @param size Size of the virtual memory region
 *
 * @retval true if all pages in the region are mapped
 * @retval false if at least one page is not mapped
 */
bool sys_mm_drv_is_virt_region_mapped(void *virt, size_t size);

/**
 * @brief Test if the virtual memory region is unmapped
 *
 * @param virt Page-aligned base virtual address
 * @param size Size of the virtual memory region
 *
 * @retval true if all pages in the region are unmapped
 * @retval false if at least one page is mapped
 */
bool sys_mm_drv_is_virt_region_unmapped(void *virt, size_t size);

/**
 * @brief Simple implementation of sys_mm_drv_map_region()
 *
 * This provides a simple implementation for sys_mm_drv_map_region()
 * which is marked as a weak alias to sys_mm_drv_map_region().
 *
 * Drivers do not have to implement their own sys_mm_drv_map_region()
 * if this works for them. Or they can override sys_mm_drv_map_region()
 * and call sys_mm_drv_simple_map_region() with some pre-processing done.
 * Or the drivers can implement their own sys_mm_drv_map_region(), then
 * this function will not be used.
 *
 * @see sys_mm_drv_map_region
 *
 * @param virt Page-aligned destination virtual address to map
 * @param phys Page-aligned source physical address to map
 * @param size Page-aligned size of the mapped memory region in bytes
 * @param flags Caching, access and control flags, see SYS_MM_MEM_* macros
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if any virtual addresses have already been mapped
 */
int sys_mm_drv_simple_map_region(void *virt, uintptr_t phys,
				 size_t size, uint32_t flags);

/**
 * @brief Simple implementation of sys_mm_drv_map_array()
 *
 * This provides a simple implementation for sys_mm_drv_map_array()
 * which is marked as a weak alias to sys_mm_drv_map_array().
 *
 * Drivers do not have to implement their own sys_mm_drv_map_array()
 * if this works for them. Or they can override sys_mm_drv_map_array()
 * and call sys_mm_drv_simple_map_array() with some pre-processing done.
 * Or the drivers can implement their own sys_mm_drv_map_array(), then
 * this function will not be used.
 *
 * @see sys_mm_drv_map_array
 *
 * @param virt Page-aligned destination virtual address to map
 * @param phys Array of pge-aligned source physical address to map
 * @param cnt Number of elements in the physical page array
 * @param flags Caching, access and control flags, see SYS_MM_MEM_* macros
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if any virtual addresses have already been mapped
 */
int sys_mm_drv_simple_map_array(void *virt, uintptr_t *phys,
				size_t cnt, uint32_t flags);

/**
 * @brief Simple implementation of sys_mm_drv_unmap_region()
 *
 * This provides a simple implementation for sys_mm_drv_unmap_region()
 * which is marked as a weak alias to sys_mm_drv_unmap_region().
 *
 * Drivers do not have to implement their own sys_mm_drv_unmap_region()
 * if this works for them. Or they can override sys_mm_drv_unmap_region()
 * and call sys_mm_drv_simple_unmap_region() with some pre-processing done.
 * Or the drivers can implement their own sys_mm_drv_unmap_region(), then
 * this function will not be used.
 *
 * @see sys_mm_drv_unmap_region
 *
 * @param virt Page-aligned base virtual address to un-map
 * @param size Page-aligned region size
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual addresses have already been mapped
 */
int sys_mm_drv_simple_unmap_region(void *virt, size_t size);

/**
 * @brief Simple implementation of sys_mm_drv_remap_region()
 *
 * This provides a simple implementation for sys_mm_drv_remap_region()
 * which is marked as a weak alias to sys_mm_drv_remap_region().
 *
 * Drivers do not have to implement their own sys_mm_drv_remap_region()
 * if this works for them. Or they can override sys_mm_drv_remap_region()
 * and call sys_mm_drv_simple_remap_region() with some pre-processing done.
 * Or the drivers can implement their own sys_mm_drv_remap_region(), then
 * this function will not be used.
 *
 * @see sys_mm_drv_remap_region
 *
 * @param virt_old Page-aligned base virtual address of existing memory
 * @param size Page-aligned size of the mapped memory region in bytes
 * @param virt_new Page-aligned base virtual address to which to remap
 *                 the memory
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if old virtual addresses are not all mapped or
 *                 new virtual addresses are not all unmapped
 */
int sys_mm_drv_simple_remap_region(void *virt_old, size_t size,
				   void *virt_new);

/**
 * @brief Simple implementation of sys_mm_drv_move_region()
 *
 * This provides a simple implementation for sys_mm_drv_move_region()
 * which is marked as a weak alias to sys_mm_drv_move_region().
 *
 * Drivers do not have to implement their own sys_mm_drv_move_region()
 * if this works for them. Or they can override sys_mm_drv_move_region()
 * and call sys_mm_drv_simple_move_region() with some pre-processing done.
 * Or the drivers can implement their own sys_mm_drv_move_region(), then
 * this function will not be used.
 *
 * @see sys_mm_drv_move_region
 *
 * @param virt_old Page-aligned base virtual address of existing memory
 * @param size Page-aligned size of the mapped memory region in bytes
 * @param virt_new Page-aligned base virtual address to which to map
 *                 new physical pages
 * @param phys_new Page-aligned base physical address to contain
 *                 the moved memory
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if old virtual addresses are not all mapped or
 *                 new virtual addresses are not all unmapped
 */
int sys_mm_drv_simple_move_region(void *virt_old, size_t size,
				  void *virt_new, uintptr_t phys_new);

/**
 * @brief Simple implementation of sys_mm_drv_move_array()
 *
 * This provides a simple implementation for sys_mm_drv_move_array()
 * which is marked as a weak alias to sys_mm_drv_move_array().
 *
 * Drivers do not have to implement their own sys_mm_drv_move_array()
 * if this works for them. Or they can override sys_mm_drv_move_array()
 * and call sys_mm_drv_simple_move_array() with some pre-processing done.
 * Or the drivers can implement their own sys_mm_drv_move_array(), then
 * this function will not be used.
 *
 * @see sys_mm_drv_move_array
 *
 * @param virt_old Page-aligned base virtual address of existing memory
 * @param size Page-aligned size of the mapped memory region in bytes
 * @param virt_new Page-aligned base virtual address to which to map
 *                 new physical pages
 * @param phys_new Array of page-aligned physical address to contain
 *                 the moved memory
 * @param phys_cnt Number of elements in the physical page array
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if old virtual addresses are not all mapped or
 *                 new virtual addresses are not all unmapped
 */
int sys_mm_drv_simple_move_array(void *virt_old, size_t size,
				 void *virt_new,
				 uintptr_t *phys_new, size_t phys_cnt);

/**
 * @brief Update memory region flags
 *
 * This changes the attributes of physical memory which is already
 * mapped to a virtual address. This is useful when use case of
 * specific memory region  changes.
 * E.g. when the library/module code is copied to the memory then
 * it needs to be read-write and after it has already
 * been copied and library/module code is ready to be executed then
 * attributes need to be changed to read-only/executable.
 * Calling this API must not cause losing memory contents.
 *
 * @param virt Page-aligned virtual address to be updated
 * @param size Page-aligned size of the mapped memory region in bytes
 * @param flags Caching, access and control flags, see SYS_MM_MEM_* macros
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual addresses is not mapped
 */

int sys_mm_drv_simple_update_region_flags(void *virt, size_t size, uint32_t flags);

#endif /* ZEPHYR_DRIVERS_SYSTEM_MM_DRV_COMMON_H_ */

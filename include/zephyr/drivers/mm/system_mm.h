/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Memory Management Driver APIs
 *
 * This contains APIs for a system-wide memory management
 * driver. Only one instance is permitted on the system.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SYSTEM_MM_H_
#define ZEPHYR_INCLUDE_DRIVERS_SYSTEM_MM_H_

#include <zephyr/kernel.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory Management Driver APIs
 * @defgroup mm_drv_apis Memory Management Driver APIs
 * @{
 */

/*
 * Caching mode definitions. These are mutually exclusive.
 */

/** No caching */
#define SYS_MM_MEM_CACHE_NONE		2

/** Write-through caching */
#define SYS_MM_MEM_CACHE_WT		1

/** Full write-back caching */
#define SYS_MM_MEM_CACHE_WB		0

/** Reserved bits for cache modes */
#define SYS_MM_MEM_CACHE_MASK		(BIT(3) - 1)

/*
 * Region permission attributes.
 * Default should be read-only, no user, no exec.
 */

/** Region will have read/write access (and not read-only) */
#define SYS_MM_MEM_PERM_RW		BIT(3)

/** Region will be executable (normally forbidden) */
#define SYS_MM_MEM_PERM_EXEC		BIT(4)

/** Region will be accessible to user mode (normally supervisor-only) */
#define SYS_MM_MEM_PERM_USER		BIT(5)

/**
 * @brief Map one physical page into the virtual address space
 *
 * This maps one physical page into the virtual address space.
 * Behavior when providing unaligned address is undefined, this
 * is assumed to be page aligned.
 *
 * The memory range itself is never accessed by this operation.
 *
 * This API must be safe to call in ISRs or exception handlers. Calls
 * to this API are assumed to be serialized.
 *
 * @param virt Page-aligned destination virtual address to map
 * @param phys Page-aligned source physical address to map
 * @param flags Caching, access and control flags, see SYS_MM_MEM_* macros
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual address has already been mapped
 */
int sys_mm_drv_map_page(void *virt, uintptr_t phys, uint32_t flags);

/**
 * @brief Map a region of physical memory into the virtual address space
 *
 * This maps a region of physical memory into the virtual address space.
 * Behavior when providing unaligned addresses/sizes is undefined, these
 * are assumed to be page aligned.
 *
 * The memory range itself is never accessed by this operation.
 *
 * This API must be safe to call in ISRs or exception handlers. Calls
 * to this API are assumed to be serialized.
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
int sys_mm_drv_map_region(void *virt, uintptr_t phys,
			  size_t size, uint32_t flags);

/**
 * @brief Map an array of physical memory into the virtual address space
 *
 * This maps an array of physical pages into a continuous virtual address
 * space. Behavior when providing unaligned addresses is undefined, these
 * are assumed to be page aligned.
 *
 * The physical memory pages are never accessed by this operation.
 *
 * This API must be safe to call in ISRs or exception handlers. Calls
 * to this API are assumed to be serialized.
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
int sys_mm_drv_map_array(void *virt, uintptr_t *phys,
			 size_t cnt, uint32_t flags);

/**
 * @brief Remove mapping for one page of the provided virtual address
 *
 * This unmaps one page from the virtual address space.
 *
 * When this completes, the relevant translation table entries will be
 * updated as if no mapping was ever made for that memory page. No previous
 * context needs to be preserved. This function must update mapping in
 * all active translation tables.
 *
 * Behavior when providing unaligned address is undefined, this
 * is assumed to be page aligned.
 *
 * Implementations must invalidate translation caching as necessary.
 *
 * @param virt Page-aligned virtual address to un-map
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual address is not mapped
 */
int sys_mm_drv_unmap_page(void *virt);

/**
 * @brief Remove mappings for a provided virtual address range
 *
 * This unmaps pages in the provided virtual address range.
 *
 * When this completes, the relevant translation table entries will be
 * updated as if no mapping was ever made for that memory range. No previous
 * context needs to be preserved. This function must update mappings in
 * all active translation tables.
 *
 * Behavior when providing unaligned address is undefined, this
 * is assumed to be page aligned.
 *
 * Implementations must invalidate translation caching as necessary.
 *
 * @param virt Page-aligned base virtual address to un-map
 * @param size Page-aligned region size
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual addresses have already been mapped
 */
int sys_mm_drv_unmap_region(void *virt, size_t size);

/**
 * @brief Get the mapped physical memory address from virtual address.
 *
 * The function queries the translation tables to find the physical
 * memory address of a mapped virtual address.
 *
 * Behavior when providing unaligned address is undefined, this
 * is assumed to be page aligned.
 *
 * @param      virt Page-aligned virtual address
 * @param[out] phys Mapped physical address (can be NULL if only checking
 *                  if virtual address is mapped)
 *
 * @retval 0 if mapping is found and valid
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual address is not mapped
 */
int sys_mm_drv_page_phys_get(void *virt, uintptr_t *phys);

/**
 * @brief Remap virtual pages into new address
 *
 * This remaps a virtual memory region starting at @p virt_old
 * of size @p size into a new virtual memory region starting at
 * @p virt_new. In other words, physical memory at @p virt_old is
 * remapped to appear at @p virt_new. Both addresses must be page
 * aligned and valid.
 *
 * Note that the virtual memory at both the old and new addresses
 * must be unmapped in the memory domains of any runnable Zephyr
 * thread as this does not deal with memory domains.
 *
 * Note that overlapping of old and new virtual memory regions
 * is usually not supported for simpler implementation. Refer to
 * the actual driver to make sure if overlapping is allowed.
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
int sys_mm_drv_remap_region(void *virt_old, size_t size, void *virt_new);

/**
 * @brief Physically move memory, with copy
 *
 * This maps a region of physical memory into the new virtual address space
 * (@p virt_new), and copy region of size @p size from the old virtual
 * address space (@p virt_old). The new virtual memory region is mapped
 * from physical memory starting at @p phys_new of size @p size.
 *
 * Behavior when providing unaligned addresses/sizes is undefined, these
 * are assumed to be page aligned.
 *
 * Note that the virtual memory at both the old and new addresses
 * must be unmapped in the memory domains of any runnable Zephyr
 * thread as this does not deal with memory domains.
 *
 * Note that overlapping of old and new virtual memory regions
 * is usually not supported for simpler implementation. Refer to
 * the actual driver to make sure if overlapping is allowed.
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
int sys_mm_drv_move_region(void *virt_old, size_t size, void *virt_new,
			   uintptr_t phys_new);

/**
 * @brief Physically move memory, with copy
 *
 * This maps a region of physical memory into the new virtual address space
 * (@p virt_new), and copy region of size @p size from the old virtual
 * address space (@p virt_old). The new virtual memory region is mapped
 * from an array of physical pages.
 *
 * Behavior when providing unaligned addresses/sizes is undefined, these
 * are assumed to be page aligned.
 *
 * Note that the virtual memory at both the old and new addresses
 * must be unmapped in the memory domains of any runnable Zephyr
 * thread as this does not deal with memory domains.
 *
 * Note that overlapping of old and new virtual memory regions
 * is usually not supported for simpler implementation. Refer to
 * the actual driver to make sure if overlapping is allowed.
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
int sys_mm_drv_move_array(void *virt_old, size_t size, void *virt_new,
			  uintptr_t *phys_new, size_t phys_cnt);


/**
 * @brief Update memory page flags
 *
 * This changes the attributes of physical memory page which is already
 * mapped to a virtual address. This is useful when use case of
 * specific memory region  changes.
 * E.g. when the library/module code is copied to the memory then
 * it needs to be read-write and after it has already
 * been copied and library/module code is ready to be executed then
 * attributes need to be changed to read-only/executable.
 * Calling this API must not cause losing memory contents.
 *
 * @param virt Page-aligned virtual address to be updated
 * @param flags Caching, access and control flags, see SYS_MM_MEM_* macros
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid arguments are provided
 * @retval -EFAULT if virtual addresses is not mapped
 */

int sys_mm_drv_update_page_flags(void *virt, uint32_t flags);

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

int sys_mm_drv_update_region_flags(void *virt, size_t size, uint32_t flags);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SYSTEM_MM_H_ */

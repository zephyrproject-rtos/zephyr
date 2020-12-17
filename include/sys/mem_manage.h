/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H
#define ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H

#include <sys/util.h>

/*
 * Caching mode definitions. These are mutually exclusive.
 */

/** No caching. Most drivers want this. */
#define K_MEM_CACHE_NONE	0

/** Write-through caching. Used by certain drivers. */
#define K_MEM_CACHE_WT		1

/** Full write-back caching. Any RAM mapped wants this. */
#define K_MEM_CACHE_WB		2

/** Reserved bits for cache modes in k_map() flags argument */
#define K_MEM_CACHE_MASK	(BIT(3) - 1)

/*
 * Region permission attributes. Default is read-only, no user, no exec
 */

/** Region will have read/write access (and not read-only) */
#define K_MEM_PERM_RW		BIT(3)

/** Region will be executable (normally forbidden) */
#define K_MEM_PERM_EXEC		BIT(4)

/** Region will be accessible to user mode (normally supervisor-only) */
#define K_MEM_PERM_USER		BIT(5)

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Map a physical memory region into the kernel's virtual address space
 *
 * Given a physical address and a size, return a linear address
 * representing the base of where the physical region is mapped in
 * the virtual address space for the Zephyr kernel.
 *
 * This function alters the active page tables in the area reserved
 * for the kernel. This function will choose the virtual address
 * and return it to the caller.
 *
 * Portable code should never assume that phys_addr and linear_addr will
 * be equal.
 *
 * Once created, mappings are permanent.
 *
 * Caching and access properties are controlled by the 'flags' parameter.
 * Unused bits in 'flags' are reserved for future expansion.
 * A caching mode must be selected. By default, the region is read-only
 * with user access and code execution forbidden. This policy is changed
 * by passing K_MEM_CACHE_* and K_MEM_PERM_* macros into the 'flags' parameter.
 *
 * If there is insufficient virtual address space for the mapping, or
 * bad flags are passed in, or if additional memory is needed to update
 * page tables that is not available, this will generate a kernel panic.
 *
 * This API is only available if CONFIG_MMU is enabled.
 *
 * This API is part of infrastructure still under development and may
 * change.
 *
 * @param virt [out] Output virtual address storage location
 * @param phys Physical address base of the memory region
 * @param size Size of the memory region
 * @param flags K_MEM_PERM_* and K_MEM_CACHE_* access and control flags
 */
void z_phys_map(uint8_t **virt_ptr, uintptr_t phys, size_t size,
		uint32_t flags);

/*
 * k_mem_map() control flags
 */

/**
 * @def K_MEM_MAP_UNINIT
 *
 * @brief The mapped region is not guaranteed to be zeroed.
 *
 * This may improve performance. The associated page frames may contain
 * indeterminate data, zeroes, or even sensitive information.
 *
 * This may not be used with K_MEM_PERM_USER as there are no circumstances
 * where this is safe.
 */
#define K_MEM_MAP_UNINIT	BIT(16)

/**
 * @def K_MEM_MAP_LOCK
 *
 * Region will be pinned in memory and never paged
 *
 * Such memory is guaranteed to never produce a page fault due to page-outs
 * or copy-on-write once the mapping call has returned. Physical page frames
 * will be pre-fetched as necessary and pinned.
 */
#define K_MEM_MAP_LOCK		BIT(17)

/**
 * @def K_MEM_MAP_GUARD
 *
 * A un-mapped virtual guard page will be placed in memory immediately preceding
 * the mapped region. This page will still be noted as being used by the
 * virtual memory manager. The total size of the allocation will be the
 * requested size plus the size of this guard page. The returned address
 * pointer will not include the guard page immediately below it. The typical
 * use-case is downward-growing thread stacks.
 *
 * Zephyr treats page faults on this guard page as a fatal K_ERR_STACK_CHK_FAIL
 * if it determines it immediately precedes a stack buffer, this is
 * implemented in the architecture layer.
 */
#define K_MEM_MAP_GUARD		BIT(18)

/**
 * Map anonymous memory into Zephyr's address space
 *
 * This function effectively increases the data space available to Zephyr.
 * The kernel will choose a base virtual address and return it to the caller.
 * The memory will have access permissions for all contexts set per the
 * provided flags argument.
 *
 * If user thread access control needs to be managed in any way, do not enable
 * K_MEM_PERM_USER flags here; instead manage the region's permissions
 * with memory domain APIs after the mapping has been established. Setting
 * K_MEM_PERM_USER here will allow all user threads to access this memory
 * which is usually undesirable.
 *
 * Unless K_MEM_MAP_UNINIT is used, the returned memory will be zeroed.
 *
 * The mapped region is not guaranteed to be physically contiguous in memory.
 * Physically contiguous buffers should be allocated statically and pinned
 * at build time.
 *
 * The returned virtual memory pointer will be page-aligned. The size
 * parameter, and any base address for re-mapping purposes must be page-
 * aligned.
 *
 * Many K_MEM_MAP_* flags have been implemented to alter the behavior of this
 * function, with details in the documentation for these flags.
 *
 * @param size Size of the memory mapping. This must be page-aligned.
 * @param flags K_MEM_CACHE_*, K_MEM_PERM_*, K_MEM_MAP_* control flags.
 * @return The mapped memory location, or NULL if insufficient virtual address
 *         space, insufficient physical memory to establish the mapping,
 *         or insufficient memory for paging structures.
 */
void *k_mem_map(size_t size, uint32_t flags);

/** Don't remove the old mapping; data pages will now have multiple mappings */
#define K_MEM_REMAP_MULTIPLE	(BIT(24) | K_MEM_MAP_LOCK)

/**
 * Re-map an existing memory mapping
 *
 * This function may be used to expand or shrink an existing memory mapping.
 * The region denoted by old_addr and old_size will be resized to fit the new
 * mapping size, and permission and caching flags will be updated.
 *
 * If the size of the new region is larger, pages will be anonymously mapped as
 * if k_mem_map() was called for them. This function otherwise does not
 * increase the data footprint of the kernel, aside from any paging structures
 * that need to be instantiated.
 *
 * The adjusted memory mapping may be moved in memory, although the kernel
 * will attempt to do the remapping in place if possible. Callers should
 * exclusively access the memory region by the returned pointer and not the
 * old one, they may be different and access via the old region may produce
 * a fatal error unless K_MEM_REMAP_MULTIPLE is used.
 *
 * If K_MEM_REMAP_MULTIPLE is provided, this will create a new mapping which
 * has the same contents as the old one, possibly with different permissions,
 * and the old region will not be un-mapped. The demand paging system is unable
 * to reverse-map page frames which have multiple mappings so this implies
 * K_MEM_MAP_LOCK. Un-pinning the associated pages frames while they still
 * are mapped is undefined behavior.
 *
 * No zeroing is performed on the remapped region. The K_MEM_MAP_UNINIT
 * parameter only applies to anonymously mapped additional pages. Great
 * care must be taken not call this with K_MEM_PERM_USER, or add the mapping
 * to a memory domain, unless the target region is definitively known not to
 * ever contain sensitive information or private kernel data.
 *
 * Calling this function on a region which was not mapped to begin with is
 * undefined behavior. It is safe to re-map boot RAM mappings in this way,
 * such as with statically allocated buffers or thread stacks in the kernel
 * image.
 *
 * @param old_addr Base page-aligned virtual address of the region to resize
 * @param old_size Base page-aligned size of the region to resize
 * @param new_size Page-aligned new size of the re-mapped region
 * @param flags K_MEM_MAP_*, K_MEM_PERM_*, K_MEM_CACHE_*, and K_MEM_REMAP_*
 *              control flags
 * @return The mapped memory location, or NULL if insufficient virtual address
 *         space, insufficient physical memory to establish the mapping,
 *         or insufficient memory for paging structures.
 */
void *k_mem_remap(void *old_addr, size_t old_size, size_t new_size,
		  uint32_t flags);

/**
 * Un-map mapped memory
 *
 * This removes a memory mapping for the provided page-aligned region.
 * Associated page frames will be free for other uses if not mapped elsewhere,
 * the kernel may re-use the associated virtual address region, and any paged
 * out data pages may be discarded.
 *
 * Calling this function on a region which was not mapped to begin with is
 * undefined behavior. It is safe to re-map boot RAM mappings in this way,
 * such as with statically allocated buffers or thread stacks in the kernel
 * image.
 *
 * @param addr Page-aligned memory region base virtual address
 * @param size Page-aligned memory region size
 */
void k_mem_unmap(void *addr, size_t size);

/**
 * Given an arbitrary region, provide a aligned region that covers it
 *
 * The returned region will have both its base address and size aligned
 * to the provided alignment value.
 *
 * @param aligned_addr [out] Aligned address
 * @param aligned_size [out] Aligned region size
 * @param addr Region base address
 * @param size Region size
 * @param align What to align the address and size to
 * @retval offset between aligned_addr and addr
 */
size_t k_mem_region_align(uintptr_t *aligned_addr, size_t *aligned_size,
			  uintptr_t addr, size_t size, size_t align);

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H */

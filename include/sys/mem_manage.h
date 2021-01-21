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
#define K_MEM_CACHE_NONE	2

/** Write-through caching. Used by certain drivers. */
#define K_MEM_CACHE_WT		1

/** Full write-back caching. Any RAM mapped wants this. */
#define K_MEM_CACHE_WB		0

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
 * This function is intended for mapping memory-mapped I/O regions into
 * the virtual address space. Given a physical address and a size, return a
 * linear address representing the base of where the physical region is mapped
 * in the virtual address space for the Zephyr kernel.
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
 * If there is insufficient virtual address space for the mapping this will
 * generate a kernel panic.
 *
 * This API is only available if CONFIG_MMU is enabled.
 *
 * It is highly discouraged to use this function to map system RAM page
 * frames. It may conflict with anonymous memory mappings and demand paging
 * and produce undefined behavior.  Do not use this for RAM unless you know
 * exactly what you are doing. If you need a chunk of memory, use k_mem_map().
 * If you need a contiguous buffer of physical memory, statically declare it
 * and pin it at build time, it will be mapped when the system boots.
 *
 * This API is part of infrastructure still under development and may
 * change.
 *
 * @param virt [out] Output virtual address storage location
 * @param phys Physical address base of the memory region
 * @param size Size of the memory region
 * @param flags Caching mode and access flags, see K_MAP_* macros
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
 * Return the amount of free memory available
 *
 * The returned value will reflect how many free RAM page frames are available.
 * If demand paging is enabled, it may still be possible to allocate more.
 *
 * The information reported by this function may go stale immediately if
 * concurrent memory mappings or page-ins take place.
 *
 * @return Free physical RAM, in bytes
 */
size_t k_mem_free_get(void);

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
 * Pages mapped in this way have write-back cache settings.
 *
 * The returned virtual memory pointer will be page-aligned. The size
 * parameter, and any base address for re-mapping purposes must be page-
 * aligned.
 *
 * Many K_MEM_MAP_* flags have been implemented to alter the behavior of this
 * function, with details in the documentation for these flags.
 *
 * @param size Size of the memory mapping. This must be page-aligned.
 * @param flags K_MEM_PERM_*, K_MEM_MAP_* control flags.
 * @return The mapped memory location, or NULL if insufficient virtual address
 *         space, insufficient physical memory to establish the mapping,
 *         or insufficient memory for paging structures.
 */
void *k_mem_map(size_t size, uint32_t flags);

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

#ifdef CONFIG_DEMAND_PAGING
/**
 * Evict a page-aligned virtual memory region to the backing store
 *
 * Useful if it is known that a memory region will not be used for some time.
 * All the data pages within the specified region will be evicted to the
 * backing store if they weren't already, with their associated page frames
 * marked as available for mappings or page-ins.
 *
 * None of the associated page frames mapped to the provided region should
 * be pinned.
 *
 * Note that there are no guarantees how long these pages will be evicted,
 * they could take page faults immediately.
 *
 * If CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled, this function may not be
 * called by ISRs as the backing store may be in-use.
 *
 * @param addr Base page-aligned virtual address
 * @param size Page-aligned data region size
 * @retval 0 Success
 * @retval -ENOMEM Insufficient space in backing store to satisfy request.
 *         The region may be partially paged out.
 */
int k_mem_page_out(void *addr, size_t size);

/**
 * Load a virtual data region into memory
 *
 * After the function completes, all the page frames associated with this
 * function will be paged in. However, they are not guaranteed to stay there.
 * This is useful if the region is known to be used soon.
 *
 * If CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled, this function may not be
 * called by ISRs as the backing store may be in-use.
 *
 * @param addr Base page-aligned virtual address
 * @param size Page-aligned data region size
 */
void k_mem_page_in(void *addr, size_t size);

/**
 * Pin an aligned virtual data region, paging in as necessary
 *
 * After the function completes, all the page frames associated with this
 * region will be resident in memory and pinned such that they stay that way.
 * This is a stronger version of z_mem_page_in().
 *
 * If CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled, this function may not be
 * called by ISRs as the backing store may be in-use.
 *
 * @param addr Base page-aligned virtual address
 * @param size Page-aligned data region size
 */
void k_mem_pin(void *addr, size_t size);

/**
 * Un-pin an aligned virtual data region
 *
 * After the function completes, all the page frames associated with this
 * region will be no longer marked as pinned. This does not evict the region,
 * follow this with z_mem_page_out() if you need that.
 *
 * @param addr Base page-aligned virtual address
 * @param size Page-aligned data region size
 */
void k_mem_unpin(void *addr, size_t size);
#endif /* CONFIG_DEMAND_PAGING */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H */

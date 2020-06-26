/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MM_H
#define ZEPHYR_INCLUDE_SYS_MM_H

#include <sys/util.h>

/*
 * Caching mode definitions. These are mutually exclusive.
 */

/** No caching. Most drivers want this. */
#define K_MAP_CACHE_NONE	0

/** Write-through caching. Used by certain drivers. */
#define K_MAP_CACHE_WT		1

/** Full write-back caching. Any RAM mapped wants this. */
#define K_MAP_CACHE_WB		2

/** Reserved bits for cache modes in k_map() flags argument */
#define K_MAP_CACHE_MASK	(BIT(3) - 1)

/*
 * Region permission attributes. Default is read-only, no user, no exec
 */

/** Region will have read/write access (and not read-only) */
#define K_MAP_RW		BIT(3)

/** Region will be executable (normally forbidden) */
#define K_MAP_EXEC		BIT(4)

/** Region will be accessible to user mode (normally supervisor-only) */
#define K_MAP_USER		BIT(5)

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MMU
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
 * by passing K_MAP_* macros into the 'flags' parameter.
 *
 * This maps memory in the kernel's part of the address space. Using
 * K_MAP_USER is almost never a good idea here and may be forbidden
 * in the future.
 *
 * If there is insufficient virtual address space for the mapping, or
 * bad flags are passed in, or if additional memory is needed to update
 * page tables that is not available, this will generate a kernel panic.
 *
 * @param linear_addr [out] Output linear address storage location
 * @param phys_addr Physical address base of the memory region
 * @param size Size of the memory region
 * @param flags Caching mode and access flags, see K_MAP_* macros
 */
void k_map(uint8_t **linear_addr, uintptr_t phys_addr, size_t size,
	   uint32_t flags);

size_t k_map_region_align(uintptr_t *aligned_addr, size_t *aligned_size,
			  uintptr_t phys_addr, size_t size);
#endif /* CONFIG_MMU */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_SYS_MM_H */

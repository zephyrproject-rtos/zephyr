/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_ADSP_MEM_H_
#define ZEPHYR_SOC_ADSP_MEM_H_

#include <zephyr/types.h>

/**
 * SOC-specific paged memory management API for cAVS hardware
 *
 * These devices implement a global PMMU allowing for limited virtual
 * address translation of physical (4k) pages across several hardware
 * types, but with different semantics than the virtual memory
 * hardware available via per-CPU MMUs.  Mappings are universal,
 * shared between all CPUs, threads and interrupt/exception contexts
 * on the system.
 *
 * The APIs here provide an allocation manager for pages within
 * physical memory regions, and a mapping utility allowing them to be
 * placed and moved within the global virtual address space.
 *
 * Note that while the hardware capabilities are insufficient for
 * implementing Zephyr userspace, the expectation is that this
 * facility will be used in concert with a MPU-based userspace
 * implementation.  So where needed, the implementation is expected to
 * integrate with the userspace internals, for example to detect
 * attempts to remap pages already mapped in runnable threads.
 *
 * @note The mapping APIs here is specificed symmetrically with
 * respect to hardware memory types.  In practice, different hardware
 * can be instantiated with different capabilities, and it may not be
 * generically possible to remap all memory identically (e.g. IMR
 * memory in particular may only be "mappable" at its physical
 * hardware address).  The specification here is agnostic as to
 * capabilities, and it is left to the application layer to understand
 * the specifics of the mapping architecture chosen and to present
 * only valid combinations to the driver layer.
 *
 * @note A note on error handling: many of these APIs are specified to
 * detect and return errors based on current mapping or userspace
 * state.  Because there are performance concerns with such checking,
 * and because these are inherently priviledged APIs, it is expected
 * that the error handling will be disableable at compile time.  This
 * allows it to be a routine part of development code but elided for
 * production binaries where needed.
 */

/**
 * @brief Audio DSP memory types
 *
 * Enumeration of hardware memory regions managed by this layer.
 */
enum soc_adsp_mem_t {
	SOC_ADSP_MEM_IMR,
	SOC_ADSP_MEM_LP_SRAM,
	SOC_ADSP_MEM_HP_SRAM,
	SOC_ADSP_MEM_NUM_TYPES
};

/**
 * @brief Hardware page ID
 *
 * Opaque identifier representing a hardware page.  The negative space
 * is reserved for error results presented via intermediate APIs
 * (e.g. -ENOMEM).  Zero is likewise reserved for use as a null value
 * within the application (but otherwise unused by the API).  All real
 * pages will have a positive definite value.  All valid RAM page
 * addresses can be represented with page IDs.
 */
typedef int32_t soc_adsp_page_id_t;

/* Hardware page size, in bytes */
#define SOC_ADSP_PAGE_SZ 4096

/**
 * @brief Hardware address of page ID
 *
 * Returns the hardware address of a given page ID
 *
 * @param pid A valid soc_adsp_page_id_t
 * @return A pointer representing the first hardware address of the page
 */
void *soc_adsp_page_addr(soc_adsp_page_id_t pid);

/**
 * @brief Page ID of physical address
 *
 * Returns the page ID containing a given physical hardware address
 *
 * @param addr A physical hardware address
 * @return The soc_adsp_page_id_t containing the specified address
 */
soc_adsp_page_id_t soc_adsp_page_id(void *addr);

/**
 * @brief Allocate hardware page
 *
 * Allocates a single hardware page of the specified type from the
 * free pool.  Returns the page ID, or -ENOMEM on error.
 *
 * @param type The memory type from which to allocate
 * @return A hardware page ID, or an error
 */
soc_adsp_page_id_t soc_adsp_page_alloc(enum soc_adsp_mem_t type);

/**
 * @brief Free hardware page
 *
 * Returns a previously allocated hardware page to its relevant pool
 * for use by other application code.  Returns zero on success, or an
 * error for an invalid page, or if the page is currently mapped by a
 * runnable thread.
 *
 * @note In most cases, pages will be freed automatically when they
 * are unmapped from the global virtual memory space.  This API exists
 * only for managing unmapped pages.
 *
 * @param page Hardware page ID to free
 * @return Zero on success, or error
 */
int32_t soc_adsp_page_free(soc_adsp_page_id_t page);

/**
 * @brief Allocate multiple hardware pages
 *
 * As for soc_adsp_page_alloc(), but allocates multiple pages at a
 * time, plaing them into the output array.
 *
 * @param type The memory type from which to allocate
 * @param count Number of pages to allocate
 * @param pages_out Output array to be populated with allocated pages
 * @return Zero on success, or -ENOMEM
 */
int32_t soc_adsp_pages_alloc(enum soc_adsp_mem_t type, size_t count,
			     soc_adsp_page_id_t *pages_out);

/**
 * @brief Free multiple pages
 *
 * As for soc_adsp_page_free(), but frees multiple pages at a time.
 *
 * @param pages Pointer to an array of pages to free
 * @param count Number of pages to free
 * @return Zero on success, or an error
 */
int32_t soc_adsp_pages_free(soc_adsp_page_id_t *pages, size_t count);

/**
 * @brief Map pages into virtual address space
 *
 * Maps pages into the global virtual address.  The virtual space
 * specified must be page aligned, currently unmapped and unused, and
 * the pages must not be in use by an existing mapping.
 *
 * @note This API is specified to fail on duplicate mappings (where
 * the same physical memory is present at multiple addresses), even
 * though the hardware is capable.  A separate "dup" API can be added
 * for such cases, but in general such a condition represents an error
 * that should be detected.
 *
 * @param count Number of pages to map
 * @param pages Array of page IDs to map
 * @param vaddr Virtual address pointer
 * @return Zero, or an error
 */
int32_t soc_adsp_pages_map(size_t count, soc_adsp_page_id_t *pages, void *vaddr);

/**
 * @brief Unmap pages from virtual address space
 *
 * Unmaps pages from the global virtual address space, and frees them
 * to their respective pools for use by other application code (as for
 * soc_adsp_page_free()).  The pages at the specified address must be
 * unmapped in the memory domain of any runnable Zephyr thread.  The
 * virtual address specified must be page aligned and valid.
 *
 * @param count Number of pages to unmap
 * @param vaddr Virtual address of pages to unmap
 * @return Zero, or an error
 */
int32_t soc_adsp_pages_unmap(size_t count, void *vaddr);

/**
 * @brief Unmap/remap virtual pages
 *
 * Acts as an atomic combination of
 * soc_adsp_pages_map()/soc_adsp_pages_unmap().  Physical memory at
 * vaddr_old is remapped to appear at vaddr_new.  Both addresses must
 * be page aligned and valid.  The virtual memory at both the old and
 * new addresses must be unmapped in the memory domains of any
 * runnable Zephyr thread.  The space at the new address must be
 * currently unmapped (i.e. overlapping remaps are not supported)
 *
 * @param count Number of pages to remap
 * @param vaddr_old Virtual address of existing memory
 * @param vaddr_old Virtual address to which to remap the memory
 * @return Zero, or an error
 */
int32_t soc_adsp_pages_remap(size_t count, void *vaddr_old, void *vaddr_new);

/**
 * @brief Physically move memory, with copy
 *
 * Atomically copies memory from existing physical pages to new
 * physical pages, optionally remapping to a different virtual
 * address.  All validity requirements are as for other APIs: the
 * virtual address must be aligned and valid, the new pages must be
 * currently unmapped, the old pages must be mapped virtually, but
 * unmapped in the memory domains of any runnable thread.
 *
 * The intent behind this API is the ability to move virtual memory
 * between different physical hardware without interfering with
 * logical application state, for example to move thread memory to
 * memory preserved across a DSP idle state.
 *
 * @note The remapping process happens logically after the copy, so
 * unlike soc_adsp_pages_remap(), the virtual addresses here are
 * allowed to overlap.
 *
 * @param count Number of pages to remap
 * @param vaddr_old Virtual address of existing memory
 * @param vaddr_old Virtual address to which to remap the memory
 * @param pages_new Array of physical pages to contain the moved memory
 * @return Zero, or an error
 */
int32_t soc_adsp_pages_copy(size_t count, void *vaddr_old, void *vaddr_new,
			    soc_adsp_page_id_t *pages_new);

/**
 * Map physical memory into the virtual address space
 *
 * This maps physical L2 HP-SRAM memory into virtual address space.
 *
 * @param vaddr Page-aligned Destination virtual address to map
 * @param paddr Page-aligned Source physical address to map
 * @param size  Page-aligned size of the mapped memory region in bytes
 * @param flags Access and control flags, see K_MEM_PERM_* macros
 */
void z_soc_mem_map(void *vaddr, uintptr_t paddr, size_t size, uint32_t flags);

/**
 * Remove mappings for a provided virtual address range
 *
 * @param vaddr Page-aligned base virtual address to un-map
 * @param size  Page-aligned region size
 */
void z_soc_mem_unmap(void *vaddr, size_t size);

/**
 * Returns the physical address associated with the (page-aligned)
 * virtual address argument
 *
 * @param vaddr Virtual page address to translate
 * @return Physical address of memory backing the page, or NULL if not mapped
 */
void *z_soc_mem_phys_addr(void *vaddr);

void soc_adsp_mem_init(void);

#endif /* ZEPHYR_SOC_ADSP_MEM_H_ */

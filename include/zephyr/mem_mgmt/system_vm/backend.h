/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System Virtual Memory Backend interface
 *
 * APIs for the system virtual memory backend which can be
 * implemented in the architecture layer, or with a custom
 * solution.
 */

#ifndef ZEPHYR_INCLUDE_MEM_MGMT_SYSTEM_VM_BACKEND_H_
#define ZEPHYR_INCLUDE_MEM_MGMT_SYSTEM_VM_BACKEND_H_

/**
 * @brief Virtual Memory Backend Interface
 * @defgroup mem_mgmt_vm_backend Virtual Memory Backend Interface
 * @ingroup mem_mgmt
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/mem_mgmt/system_vm/priv.h>

#if defined(CONFIG_MEM_MGMT_VM_BACKEND_ARCH)

/* Forward to the architecture-specific implementation. */
#include <zephyr/mem_mgmt/system_vm/backend_arch.h>

#elif defined(CONFIG_MEM_MGMT_VM_BACKEND_CUSTOM)

/* Custom VM backend implementation.
 *
 * This header needs to be present in search path.
 *
 * Needs to define these macros in the header file so they can be used by
 * kernel and demand paging eviction algorithms:
 *   SYS_MM_VM_DATA_PAGE_LOADED
 *   SYS_MM_VM_DATA_PAGE_ACCESSED
 *   SYS_MM_VM_DATA_PAGE_DIRTY
 *   SYS_MM_VM_DATA_PAGE_NOT_MAPPED
 */
#include <sys_mm_vm_backend_custom.h>

#elif defined(CONFIG_MEM_MGMT_VM_BACKEND_NONE)

/* No VM backend. All sys_mm_vm_backend_*() are not available. */

#else
#error "No CONFIG_MEM_MGMT_VM_BACKEND_* backend selected"
#endif

/**
 * Map a physical memory region into the virtual address space.
 *
 * This is a low-level interface to mapping pages into the address space.
 * Behavior when providing unaligned addresses/sizes is undefined, these
 * are assumed to be aligned to CONFIG_MMU_PAGE_SIZE.
 *
 * Management of virtual address space is handled outside of this function.
 * By the time this function is invoked, we know exactly where this mapping
 * will be established. If the page tables already had mappings installed
 * for the virtual memory region, they will be overwritten.
 *
 * The memory range itself is never accessed by this operation.
 *
 * This API must be safe to call in ISRs or exception handlers. Calls
 * to this API are assumed to be serialized, and indeed all usage will
 * originate from functions which handles virtual memory management.
 *
 * The backend must never require allocation for paging structures; page tables
 * for the whole address space are expected to be pre-allocated.
 *
 * Validation of arguments should be done via assertions.
 *
 * @param virt Page-aligned destination virtual address to map
 * @param phys Page-aligned source physical address to map
 * @param size Page-aligned size of the mapped region in bytes
 * @param flags Caching, access and control flags (see K_MEM_* macros)
 */
void sys_mm_vm_backend_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags);

/**
 * Remove mappings for a provided virtual address range
 *
 * This is a low-level interface for un-mapping pages from the address space.
 * When this completes, the relevant page table entries will be updated as
 * if no mapping was ever made for that memory range. No previous context
 * needs to be preserved. This function must update mappings in all active
 * page tables.
 *
 * Behavior when providing unaligned addresses/sizes is undefined, these
 * are assumed to be aligned to CONFIG_MMU_PAGE_SIZE.
 *
 * Behavior when providing an address range that is not already mapped is
 * undefined.
 *
 * This function should never require memory allocations for paging structures,
 * and it is not necessary to free any paging structures. Empty page tables
 * due to all contained entries being un-mapped may remain in place.
 *
 * Implementations must invalidate TLBs as necessary.
 *
 * @param addr Page-aligned base virtual address to un-map
 * @param size Page-aligned region size
 */
void sys_mm_vm_backend_mem_unmap(void *addr, size_t size);

/**
 * Get the mapped physical memory address from virtual address.
 *
 * The function only needs to query the current set of page tables as
 * the information it reports must be common to all of them if multiple
 * page tables are in use. If multiple page tables are active it is unnecessary
 * to iterate over all of them.
 *
 * Unless otherwise specified, virtual pages have the same mappings
 * across all page tables. Calling this function on data pages that are
 * exceptions to this rule (such as the scratch page) is undefined behavior.
 *
 * @param virt Page-aligned virtual address
 * @param[out] phys Mapped physical address (can be NULL if only checking
 *                  if virtual address is mapped)
 *
 * @retval 0 if mapping is found and valid
 * @retval -EFAULT if virtual address is not mapped
 */
int sys_mm_vm_backend_page_phys_get(void *virt, uintptr_t *phys);

/**
 * Mark reserved page frames in the page frame database.
 *
 * Some page frames within system RAM are not available for use (for example
 * reserved regions in the first megabyte on PC-like systems).
 *
 * Implementations of this function should mark all relevant entries in
 * k_mem_page_frames with K_PAGE_FRAME_RESERVED. This function is called at
 * early system initialization.
 */
void sys_mm_vm_backend_reserved_pages_update(void);

/**
 * Update all page tables for a paged-out data page
 *
 * This function:
 * - Sets the data page virtual address to trigger a fault if accessed that
 *   can be distinguished from access violations or un-mapped pages.
 * - Saves the provided location value so that it can retrieved for that
 *   data page in the page fault handler.
 * - The location value semantics are undefined here but the value will be
 *   always be page-aligned. It could be 0.
 *
 * If multiple page tables are in use, this must update all page tables.
 * This function is called with interrupts locked.
 *
 * Calling this function on data pages which are already paged out is
 * undefined behavior.
 *
 * @param addr Virtual data page address being evicted
 * @param location Backing store location value to associate with the page
 */
void sys_mm_vm_backend_mem_page_out(void *addr, uintptr_t location);

/**
 * Update all page tables for a paged-in data page
 *
 * This function:
 * - Maps the specified virtual data page address to the provided physical
 *   page frame address, such that future memory accesses will function as
 *   expected. Access and caching attributes are undisturbed.
 * - Clears any accounting for "accessed" and "dirty" states.
 *
 * @param addr Virtual data page address being paged in
 * @param phys Physical page frame address to map it to
 */
void sys_mm_vm_backend_mem_page_in(void *addr, uintptr_t phys);

/**
 * Temporarily map a physical page frame for scratch access.
 *
 * Map the physical page @p phys to a special virtual address
 * K_MEM_SCRATCH_PAGE, with supervisor read/write access, such that
 * when this function returns, the calling context can read/write the page
 * frame's contents from the K_MEM_SCRATCH_PAGE address.
 *
 * This mapping only needs to be done on the current set of page tables,
 * as it is only used for a short period of time exclusively by the caller.
 * This function is called with interrupts locked.
 *
 * @param phys Physical page frame address to map at the scratch page
 */
void sys_mm_vm_backend_mem_scratch(uintptr_t phys);

/**
 * Fetch location information about a page at a particular address
 *
 * The function only needs to query the current set of page tables as
 * the information it reports must be common to all of them if multiple
 * page tables are in use. If multiple page tables are active it is unnecessary
 * to iterate over all of them.
 *
 * This function is called with interrupts locked, so that the reported
 * information can't become stale while decisions are being made based on it.
 *
 * Unless otherwise specified, virtual data pages have the same mappings
 * across all page tables. Calling this function on data pages that are
 * exceptions to this rule (such as the scratch page) is undefined behavior.
 * Just check the currently installed page tables and return the information
 * in that.
 *
 * @param addr Virtual data page address that took the page fault
 * @param[out] location In the case of ::SYS_MM_VM_PAGE_LOCATION_PAGED_OUT, the backing store
 *                      location value used to retrieve the data page. In the case of
 *                      ::SYS_MM_VM_PAGE_LOCATION_PAGED_IN, the physical address the page is
 *                      mapped to.
 *
 * @retval SYS_MM_VM_PAGE_LOCATION_PAGED_OUT The page was evicted to the backing store
 * @retval SYS_MM_VM_PAGE_LOCATION_PAGED_IN The page is resident in memory
 * @retval SYS_MM_VM_PAGE_LOCATION_BAD The page is un-mapped or otherwise has had invalid access
 */
enum sys_mm_vm_page_location sys_mm_vm_backend_page_location_get(void *addr, uintptr_t *location);

/**
 * Retrieve page characteristics from the page table(s)
 *
 * The backend is responsible for maintaining "accessed" and "dirty"
 * states of data pages to support marking eviction algorithms. This can
 * either be directly supported by hardware or emulated by modifying
 * protection policy to generate faults on reads or writes. In all cases
 * the backend must maintain this information in some way.
 *
 * For the provided virtual address, report the logical OR of the accessed
 * and dirty states for the relevant entries in all active page tables in
 * the system if the page is mapped and not paged out.
 *
 * If @p clear_accessed is true, the ::SYS_MM_VM_DATA_PAGE_ACCESSED flag will be
 * reset. This function will report its prior state. If multiple page tables are in
 * use, this function clears accessed state in all of them.
 *
 * This function is called with interrupts locked, so that the reported
 * information can't become stale while decisions are being made based on it.
 *
 * The return value may have other bits set which the caller must ignore.
 *
 * Clearing accessed state for data pages that are not ::SYS_MM_VM_DATA_PAGE_LOADED
 * is undefined behavior.
 *
 * ::SYS_MM_VM_DATA_PAGE_DIRTY and ::SYS_MM_VM_DATA_PAGE_ACCESSED bits in the return
 * value are only significant if ::SYS_MM_VM_DATA_PAGE_LOADED is set, otherwise ignore
 * them.
 *
 * ::SYS_MM_VM_DATA_PAGE_NOT_MAPPED bit in the return value is only significant
 * if ::SYS_MM_VM_DATA_PAGE_LOADED is un-set, otherwise ignore it.
 *
 * Unless otherwise specified, virtual data pages have the same mappings
 * across all page tables. Calling this function on data pages that are
 * exceptions to this rule (such as the scratch page) is undefined behavior.
 *
 * @param addr Virtual address to look up in page tables
 * @param[out] location If non-NULL, updated with either physical page frame
 *                      address or backing store location depending on
 *                      ::SYS_MM_VM_DATA_PAGE_LOADED state. This is not touched if
 *                      ::SYS_MM_VM_DATA_PAGE_NOT_MAPPED.
 * @param clear_accessed Whether to clear ::SYS_MM_VM_DATA_PAGE_ACCESSED state
 *
 * @retval Value with SYS_MM_VM_DATA_PAGE_* bits set reflecting the data page
 *         configuration
 */
uintptr_t sys_mm_vm_backend_mem_page_info_get(void *addr, uintptr_t *location, bool clear_accessed);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_MEM_MGMT_SYSTEM_VM_BACKEND_H_ */

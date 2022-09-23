/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H
#define ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#if defined(CONFIG_ARM_MMU) && defined(CONFIG_ARM64)
#include <zephyr/arch/arm64/arm_mem.h>
#endif

/**
 * @brief Memory Management
 * @defgroup memory_management Memory Management
 * @{
 * @}
 */

/*
 * Caching mode definitions. These are mutually exclusive.
 */

/** No caching. Most drivers want this. */
#define K_MEM_CACHE_NONE	2

/** Write-through caching. Used by certain drivers. */
#define K_MEM_CACHE_WT		1

/** Full write-back caching. Any RAM mapped wants this. */
#define K_MEM_CACHE_WB		0

/*
 * ARM64 Specific flags are defined in arch/arm64/arm_mem.h,
 * pay attention to be not conflicted when updating these flags.
 */

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

/*
 * This is the offset to subtract from a virtual address mapped in the
 * kernel's permanent mapping of RAM, to obtain its physical address.
 *
 *     virt_addr = phys_addr + Z_MEM_VM_OFFSET
 *
 * This only works for virtual addresses within the interval
 * [CONFIG_KERNEL_VM_BASE, CONFIG_KERNEL_VM_BASE + (CONFIG_SRAM_SIZE * 1024)).
 *
 * These macros are intended for assembly, linker code, and static initializers.
 * Use with care.
 *
 * Note that when demand paging is active, these will only work with page
 * frames that are pinned to their virtual mapping at boot.
 *
 * TODO: This will likely need to move to an arch API or need additional
 * constraints defined.
 */
#ifdef CONFIG_MMU
#define Z_MEM_VM_OFFSET	((CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_OFFSET) - \
			 (CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_OFFSET))
#else
#define Z_MEM_VM_OFFSET	0
#endif

#define Z_MEM_PHYS_ADDR(virt)	((virt) - Z_MEM_VM_OFFSET)
#define Z_MEM_VIRT_ADDR(phys)	((phys) + Z_MEM_VM_OFFSET)

#if Z_MEM_VM_OFFSET != 0
#define Z_VM_KERNEL 1
#ifdef CONFIG_XIP
#error "XIP and a virtual memory kernel are not allowed"
#endif
#endif

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <zephyr/sys/__assert.h>

struct k_mem_paging_stats_t {
#ifdef CONFIG_DEMAND_PAGING_STATS
	struct {
		/** Number of page faults */
		unsigned long			cnt;

		/** Number of page faults with IRQ locked */
		unsigned long			irq_locked;

		/** Number of page faults with IRQ unlocked */
		unsigned long			irq_unlocked;

#ifndef CONFIG_DEMAND_PAGING_ALLOW_IRQ
		/** Number of page faults while in ISR */
		unsigned long			in_isr;
#endif
	} pagefaults;

	struct {
		/** Number of clean pages selected for eviction */
		unsigned long			clean;

		/** Number of dirty pages selected for eviction */
		unsigned long			dirty;
	} eviction;
#endif /* CONFIG_DEMAND_PAGING_STATS */
};

struct k_mem_paging_histogram_t {
#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
	/* Counts for each bin in timing histogram */
	unsigned long	counts[CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS];

	/* Bounds for the bins in timing histogram,
	 * excluding the first and last (hence, NUM_SLOTS - 1).
	 */
	unsigned long	bounds[CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS];
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */
};

/* Just like Z_MEM_PHYS_ADDR() but with type safety and assertions */
static inline uintptr_t z_mem_phys_addr(void *virt)
{
	uintptr_t addr = (uintptr_t)virt;

#ifdef CONFIG_MMU
	__ASSERT((addr >= CONFIG_KERNEL_VM_BASE) &&
		 (addr < (CONFIG_KERNEL_VM_BASE +
			  (CONFIG_KERNEL_VM_SIZE))),
		 "address %p not in permanent mappings", virt);
#else
	/* Should be identity-mapped */
	__ASSERT((addr >= CONFIG_SRAM_BASE_ADDRESS) &&
		 (addr < (CONFIG_SRAM_BASE_ADDRESS +
			  (CONFIG_SRAM_SIZE * 1024UL))),
		 "physical address 0x%lx not in RAM",
		 (unsigned long)addr);
#endif /* CONFIG_MMU */

	/* TODO add assertion that this page is pinned to boot mapping,
	 * the above checks won't be sufficient with demand paging
	 */

	return Z_MEM_PHYS_ADDR(addr);
}

/* Just like Z_MEM_VIRT_ADDR() but with type safety and assertions */
static inline void *z_mem_virt_addr(uintptr_t phys)
{
	__ASSERT((phys >= CONFIG_SRAM_BASE_ADDRESS) &&
		 (phys < (CONFIG_SRAM_BASE_ADDRESS +
			  (CONFIG_SRAM_SIZE * 1024UL))),
		 "physical address 0x%lx not in RAM", (unsigned long)phys);

	/* TODO add assertion that this page frame is pinned to boot mapping,
	 * the above check won't be sufficient with demand paging
	 */

	return (void *)Z_MEM_VIRT_ADDR(phys);
}

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

/**
 * Unmap a virtual memory region from kernel's virtual address space.
 *
 * This function is intended to be used by drivers and early boot routines
 * where temporary memory mappings need to be made. This allows these
 * memory mappings to be discarded once they are no longer needed.
 *
 * This function alters the active page tables in the area reserved
 * for the kernel.
 *
 * This will align the input parameters to page boundaries so that
 * this can be used with the virtual address as returned by
 * z_phys_map().
 *
 * This API is only available if CONFIG_MMU is enabled.
 *
 * It is highly discouraged to use this function to unmap memory mappings.
 * It may conflict with anonymous memory mappings and demand paging and
 * produce undefined behavior. Do not use this unless you know exactly
 * what you are doing.
 *
 * This API is part of infrastructure still under development and may
 * change.
 *
 * @param virt Starting address of the virtual address region to be unmapped.
 * @param size Size of the virtual address region
 */
void z_phys_unmap(uint8_t *virt, size_t size);

/*
 * k_mem_map() control flags
 */

/**
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
 * Region will be pinned in memory and never paged
 *
 * Such memory is guaranteed to never produce a page fault due to page-outs
 * or copy-on-write once the mapping call has returned. Physical page frames
 * will be pre-fetched as necessary and pinned.
 */
#define K_MEM_MAP_LOCK		BIT(17)

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
 * Note that the allocation includes two guard pages immediately before
 * and after the requested region. The total size of the allocation will be
 * the requested size plus the size of these two guard pages.
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
 * Un-map mapped memory
 *
 * This removes a memory mapping for the provided page-aligned region.
 * Associated page frames will be free and the kernel may re-use the associated
 * virtual address region. Any paged out data pages may be discarded.
 *
 * Calling this function on a region which was not mapped to begin with is
 * undefined behavior.
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

/**
 * @defgroup demand_paging Demand Paging
 * @ingroup memory_management
 */
/**
 * @defgroup mem-demand-paging Demand Paging APIs
 * @ingroup demand_paging
 * @{
 */

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

/**
 * Get the paging statistics since system startup
 *
 * This populates the paging statistics struct being passed in
 * as argument.
 *
 * @param[in,out] stats Paging statistics struct to be filled.
 */
__syscall void k_mem_paging_stats_get(struct k_mem_paging_stats_t *stats);

struct k_thread;
/**
 * Get the paging statistics since system startup for a thread
 *
 * This populates the paging statistics struct being passed in
 * as argument for a particular thread.
 *
 * @param[in] thread Thread
 * @param[in,out] stats Paging statistics struct to be filled.
 */
__syscall
void k_mem_paging_thread_stats_get(struct k_thread *thread,
				   struct k_mem_paging_stats_t *stats);

/**
 * Get the eviction timing histogram
 *
 * This populates the timing histogram struct being passed in
 * as argument.
 *
 * @param[in,out] hist Timing histogram struct to be filled.
 */
__syscall void k_mem_paging_histogram_eviction_get(
	struct k_mem_paging_histogram_t *hist);

/**
 * Get the backing store page-in timing histogram
 *
 * This populates the timing histogram struct being passed in
 * as argument.
 *
 * @param[in,out] hist Timing histogram struct to be filled.
 */
__syscall void k_mem_paging_histogram_backing_store_page_in_get(
	struct k_mem_paging_histogram_t *hist);

/**
 * Get the backing store page-out timing histogram
 *
 * This populates the timing histogram struct being passed in
 * as argument.
 *
 * @param[in,out] hist Timing histogram struct to be filled.
 */
__syscall void k_mem_paging_histogram_backing_store_page_out_get(
	struct k_mem_paging_histogram_t *hist);

#include <syscalls/mem_manage.h>

/** @} */

/**
 * Eviction algorithm APIs
 *
 * @defgroup mem-demand-paging-eviction Eviction Algorithm APIs
 * @ingroup demand_paging
 * @{
 */

/**
 * Select a page frame for eviction
 *
 * The kernel will invoke this to choose a page frame to evict if there
 * are no free page frames.
 *
 * This function will never be called before the initial
 * k_mem_paging_eviction_init().
 *
 * This function is invoked with interrupts locked.
 *
 * @param [out] dirty Whether the page to evict is dirty
 * @return The page frame to evict
 */
struct z_page_frame *k_mem_paging_eviction_select(bool *dirty);

/**
 * Initialization function
 *
 * Called at POST_KERNEL to perform any necessary initialization tasks for the
 * eviction algorithm. k_mem_paging_eviction_select() is guaranteed to never be
 * called until this has returned, and this will only be called once.
 */
void k_mem_paging_eviction_init(void);

/** @} */

/**
 * Backing store APIs
 *
 * @defgroup mem-demand-paging-backing-store Backing Store APIs
 * @ingroup demand_paging
 * @{
 */

/**
 * Reserve or fetch a storage location for a data page loaded into a page frame
 *
 * The returned location token must be unique to the mapped virtual address.
 * This location will be used in the backing store to page out data page
 * contents for later retrieval. The location value must be page-aligned.
 *
 * This function may be called multiple times on the same data page. If its
 * page frame has its Z_PAGE_FRAME_BACKED bit set, it is expected to return
 * the previous backing store location for the data page containing a cached
 * clean copy. This clean copy may be updated on page-out, or used to
 * discard clean pages without needing to write out their contents.
 *
 * If the backing store is full, some other backing store location which caches
 * a loaded data page may be selected, in which case its associated page frame
 * will have the Z_PAGE_FRAME_BACKED bit cleared (as it is no longer cached).
 *
 * pf->addr will indicate the virtual address the page is currently mapped to.
 * Large, sparse backing stores which can contain the entire address space
 * may simply generate location tokens purely as a function of pf->addr with no
 * other management necessary.
 *
 * This function distinguishes whether it was called on behalf of a page
 * fault. A free backing store location must always be reserved in order for
 * page faults to succeed. If the page_fault parameter is not set, this
 * function should return -ENOMEM even if one location is available.
 *
 * This function is invoked with interrupts locked.
 *
 * @param pf Virtual address to obtain a storage location
 * @param [out] location storage location token
 * @param page_fault Whether this request was for a page fault
 * @return 0 Success
 * @return -ENOMEM Backing store is full
 */
int k_mem_paging_backing_store_location_get(struct z_page_frame *pf,
					    uintptr_t *location,
					    bool page_fault);

/**
 * Free a backing store location
 *
 * Any stored data may be discarded, and the location token associated with
 * this address may be re-used for some other data page.
 *
 * This function is invoked with interrupts locked.
 *
 * @param location Location token to free
 */
void k_mem_paging_backing_store_location_free(uintptr_t location);

/**
 * Copy a data page from Z_SCRATCH_PAGE to the specified location
 *
 * Immediately before this is called, Z_SCRATCH_PAGE will be mapped read-write
 * to the intended source page frame for the calling context.
 *
 * Calls to this and k_mem_paging_backing_store_page_in() will always be
 * serialized, but interrupts may be enabled.
 *
 * @param location Location token for the data page, for later retrieval
 */
void k_mem_paging_backing_store_page_out(uintptr_t location);

/**
 * Copy a data page from the provided location to Z_SCRATCH_PAGE.
 *
 * Immediately before this is called, Z_SCRATCH_PAGE will be mapped read-write
 * to the intended destination page frame for the calling context.
 *
 * Calls to this and k_mem_paging_backing_store_page_out() will always be
 * serialized, but interrupts may be enabled.
 *
 * @param location Location token for the data page
 */
void k_mem_paging_backing_store_page_in(uintptr_t location);

/**
 * Update internal accounting after a page-in
 *
 * This is invoked after k_mem_paging_backing_store_page_in() and interrupts
 * have been* re-locked, making it safe to access the z_page_frame data.
 * The location value will be the same passed to
 * k_mem_paging_backing_store_page_in().
 *
 * The primary use-case for this is to update custom fields for the backing
 * store in the page frame, to reflect where the data should be evicted to
 * if it is paged out again. This may be a no-op in some implementations.
 *
 * If the backing store caches paged-in data pages, this is the appropriate
 * time to set the Z_PAGE_FRAME_BACKED bit. The kernel only skips paging
 * out clean data pages if they are noted as clean in the page tables and the
 * Z_PAGE_FRAME_BACKED bit is set in their associated page frame.
 *
 * @param pf Page frame that was loaded in
 * @param location Location of where the loaded data page was retrieved
 */
void k_mem_paging_backing_store_page_finalize(struct z_page_frame *pf,
					      uintptr_t location);

/**
 * Backing store initialization function.
 *
 * The implementation may expect to receive page in/out calls as soon as this
 * returns, but not before that. Called at POST_KERNEL.
 *
 * This function is expected to do two things:
 * - Initialize any internal data structures and accounting for the backing
 *   store.
 * - If the backing store already contains all or some loaded kernel data pages
 *   at boot time, Z_PAGE_FRAME_BACKED should be appropriately set for their
 *   associated page frames, and any internal accounting set up appropriately.
 */
void k_mem_paging_backing_store_init(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_SYS_MEM_MANAGE_H */

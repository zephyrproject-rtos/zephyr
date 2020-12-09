/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef KERNEL_INCLUDE_MMU_H
#define KERNEL_INCLUDE_MMU_H

#include <stdint.h>
#include <sys/slist.h>

/*
 * Macros and data structures for physical page frame accounting,
 * APIs for use by eviction and backing store algorithms. This code
 * is otherwise not application-facing.
 */

/*
 * z_page_frame flags bits
 */

/** This page contains critical kernel data and will never be swapped */
#define Z_PAGE_FRAME_PINNED		BIT(0)

/** This physical page is reserved by hardware; we will never use it */
#define Z_PAGE_FRAME_RESERVED		BIT(1)

/**
 * This physical page is mapped to some virtual memory address
 *
 * Currently, we just support one mapping per page frame. If a page frame
 * is mapped to multiple virtual pages then it must be pinned.
 */
#define Z_PAGE_FRAME_MAPPED		BIT(2)

/**
 * This page frame is currently involved in a page-in/out operation
 */
#define Z_PAGE_FRAME_BUSY		BIT(3)

/**
 * Data structure for physical page frames
 *
 * An array of these is instantiated, one element per physical RAM page.
 * Hence it's necessary to constrain its size as much as possible.
 */
struct z_page_frame {
	union {
		/* If mapped, virtual address this page is mapped to */
		void *addr;

		/* If unmapped and available, free pages list membership. */
		sys_dnode_t node;
	};

	/* Bits 0-15 reserved for kernel Z_PAGE_FRAME_* flags
	 * Bits 16-31 reserved for page eviction algorithm implementation, if
	 * needed.
	 */
	uint32_t flags;

#ifdef CONFIG_DEMAND_PAGING
	/* TODO: Additional data structures may go here specific to the page
	 * eviction algorithm in use.
	 */
#endif
};

static inline bool z_page_frame_is_pinned(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_PINNED) != 0;
}

static inline bool z_page_frame_is_reserved(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_RESERVED) != 0;
}

static inline bool z_page_frame_is_mapped(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_MAPPED) != 0;
}

static inline bool z_page_frame_is_busy(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_BUSY) != 0;
}

static inline bool z_page_frame_is_evictable(struct z_page_frame *pf)
{
	return (!z_page_frame_is_reserved(pf) && z_page_frame_is_mapped(pf) &&
		!z_page_frame_is_pinned(pf) && !z_page_frame_is_busy(pf));
}

/* If true, page is not being used for anything, is not reserved, is a member
 * of some free pages list, isn't busy, and may be mapped in memory
 */
static inline bool z_page_frame_is_available(struct z_page_frame *page)
{
	return (page->flags & 0xFFFFU) == 0;
}

static inline void z_assert_phys_aligned(uintptr_t phys)
{
	__ASSERT(phys % CONFIG_MMU_PAGE_SIZE == 0,
		 "physical address 0x%lu is not page-aligned", phys);
	(void)phys;
}

/*
 * At present, page frame management is only done for main system RAM,
 * and we generate paging structures based on CONFIG_SRAM_BASE_ADDRESS
 * and CONFIG_SRAM_SIZE.
 *
 * If we have other RAM regions (DCCM, etc) these typically have special
 * properties and shouldn't be used generically for demand paging or
 * anonymous mappings. We don't currently maintain an ontology of these in the
 * core kernel.
 */
#define Z_NUM_PAGE_FRAMES (KB(CONFIG_SRAM_SIZE) / CONFIG_MMU_PAGE_SIZE)

#define Z_PHYS_RAM_END	(CONFIG_SRAM_BASE_ADDRESS + KB(CONFIG_SRAM_SIZE))
#define Z_VIRT_RAM_END	(CONFIG_KERNEL_VM_BASE + KB(CONFIG_SRAM_SIZE))

extern struct z_page_frame z_page_frames[Z_NUM_PAGE_FRAMES];

static inline uintptr_t z_page_frame_to_phys(struct z_page_frame *pf)
{
	return (uintptr_t)((pf - z_page_frames) * CONFIG_MMU_PAGE_SIZE) +
			CONFIG_SRAM_BASE_ADDRESS;
}

static inline bool z_is_page_frame(uintptr_t phys)
{
	z_assert_phys_aligned(phys);
	return (phys >= CONFIG_SRAM_BASE_ADDRESS) && (phys < Z_PHYS_RAM_END);
}

static inline struct z_page_frame *z_phys_to_page_frame(uintptr_t phys)
{
	__ASSERT(z_is_page_frame(phys),
		 "0x%lx not an SRAM physical address", phys);

	return &z_page_frames[(phys - CONFIG_SRAM_BASE_ADDRESS) /
			      CONFIG_MMU_PAGE_SIZE];
}

/* Convenience macro for iterating over all page frames */
#define Z_PAGE_FRAME_FOREACH(_phys, _pageframe) \
	for (_phys = CONFIG_SRAM_BASE_ADDRESS, _pageframe = z_page_frames; \
	     _phys < Z_PHYS_RAM_END; \
	     _phys += CONFIG_MMU_PAGE_SIZE, _pageframe++)

#ifdef CONFIG_DEMAND_PAGING
/* We reserve a virtual page as a scratch area for page-ins/outs at the end
 * of the address space
 */
#define Z_VM_RESERVED	CONFIG_MMU_PAGE_SIZE
#define Z_SCRATCH_PAGE	((void *)((uintptr_t)CONFIG_KERNEL_VM_BASE + \
				     (uintptr_t)CONFIG_KERNEL_VM_SIZE - \
				     CONFIG_MMU_PAGE_SIZE))
#else
#define Z_VM_RESERVED	0
#endif

#ifdef CONFIG_DEMAND_PAGING
/*
 * Eviction algorihm APIs
 */

/**
 * Select a page frame for eviction
 *
 * The kernel will invoke this to choose a page frame to evict if there
 * are no free page frames.
 *
 * This function will never be called before the initial z_eviction_init().
 *
 * This function is invoked with interrupts locked.
 *
 * @param [out] Whether the page to evict is dirty
 * @return The page frame to evict
 */
struct z_page_frame *z_eviction_select(bool *dirty);

/**
 * Initialization function
 *
 * Called at early boot to perform any necessary initialization tasks for the
 * eviction algorithm. z_eviction_init() is guaranteed to never be called
 * until this has returned, and this will only be called once.
 */
void z_eviction_init(void);

/*
 * Backing store APIs
 */

/**
 * Reserve or fetch a storage location for a data page
 *
 * The returned location token must be:
 * 1) Unique to the provided virtual address
 * 2) Aligned to CONFIG_MMU_PAGE_SIZE
 * 3) Consistent between calls, unless z_backing_store_location_free() has
 *    been invoked, in which case any associated data at the storage location
 *    will be discarded.
 *
 * This location will be used in the backing store to page out data page
 * contents for later retrieval.
 *
 * @param addr Data page virtual address
 * @return storage location token
 */
uintptr_t z_backing_store_location_get(void *addr);

/**
 * Free a backing store location for a virtual data page
 *
 * Any stored data may be discarded, and the location token associated with
 * this address may be re-used for some other data page.
 *
 * @param addr Data page virtual address
 */
void z_backing_store_location_free(void *addr);

/**
 * Copy a data page from Z_SCRATCH_PAGE to the specified location
 *
 * Immediately before this is called, Z_SCRATCH_PAGE will be mapped read-write
 * to the intended source page frame for the calling context.
 *
 * Calls to this and z_backing_store_page_in() will always be serialized,
 * but interrupts may be enabled.
 *
 * @param location Location token for the data page, for later retrieval
 */
void z_backing_store_page_out(uintptr_t location);

/**
 * Copy a data page from the provided location to Z_SCRATCH_PAGE.
 *
 * Immediately before this is called, Z_SCRATCH_PAGE will be mapped read-write
 * to the intended destination page frame for the calling context.
 *
 * Calls to this and z_backing_store_page_out() will always be serialized,
 * but interrupts may be enabled.
 *
 * @param location Location token for the data page
 */
void z_backing_store_page_in(uintptr_t location);

/**
 * Backing store initialization function.
 *
 * The implementation may expect to receive page in/out calls as soon as this
 * returns, but not before that. Called at early kernel initialization.
 */
void z_backing_store_init(void);

/*
 * Core kernel demand paging APIs
 */

/**
 * Free a page frame physical address by evicting its contents
 *
 * The indicated page frame, if it contains a data page, will have that
 * data page evicted to the backing store. The page frame will then be
 * marked as available for mappings or page-ins.
 *
 * This is useful for freeing up entire memory banks so that they may be
 * deactivated to save power.
 *
 * If CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled, this function may not be
 * called by ISRs as the backing store may be in-use.
 *
 * @param phys Page frame physical address
 */
void z_page_frame_evict(uintptr_t phys);
#endif /* CONFIG_DEMAND_PAGING */
#endif /* KERNEL_INCLUDE_MMU_H */

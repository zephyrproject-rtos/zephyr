/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef KERNEL_INCLUDE_MMU_H
#define KERNEL_INCLUDE_MMU_H

#ifdef CONFIG_MMU

#include <stdint.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/linker/linker-defs.h>

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
#define Z_PHYS_RAM_START	((uintptr_t)CONFIG_SRAM_BASE_ADDRESS)
#define Z_PHYS_RAM_SIZE		((size_t)KB(CONFIG_SRAM_SIZE))
#define Z_PHYS_RAM_END		(Z_PHYS_RAM_START + Z_PHYS_RAM_SIZE)
#define Z_NUM_PAGE_FRAMES	(Z_PHYS_RAM_SIZE / (size_t)CONFIG_MMU_PAGE_SIZE)

/** End virtual address of virtual address space */
#define Z_VIRT_RAM_START	((uint8_t *)CONFIG_KERNEL_VM_BASE)
#define Z_VIRT_RAM_SIZE		((size_t)CONFIG_KERNEL_VM_SIZE)
#define Z_VIRT_RAM_END		(Z_VIRT_RAM_START + Z_VIRT_RAM_SIZE)

/* Boot-time virtual location of the kernel image. */
#define Z_KERNEL_VIRT_START	((uint8_t *)(&z_mapped_start))
#define Z_KERNEL_VIRT_END	((uint8_t *)(&z_mapped_end))
#define Z_KERNEL_VIRT_SIZE	(Z_KERNEL_VIRT_END - Z_KERNEL_VIRT_START)

#define Z_VM_OFFSET	 ((CONFIG_KERNEL_VM_BASE + CONFIG_KERNEL_VM_OFFSET) - \
			  (CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_OFFSET))

/* Only applies to boot RAM mappings within the Zephyr image that have never
 * been remapped or paged out. Never use this unless you know exactly what you
 * are doing.
 */
#define Z_BOOT_VIRT_TO_PHYS(virt) ((uintptr_t)(((uint8_t *)virt) - Z_VM_OFFSET))
#define Z_BOOT_PHYS_TO_VIRT(phys) ((uint8_t *)(((uintptr_t)phys) + Z_VM_OFFSET))

#ifdef CONFIG_ARCH_MAPS_ALL_RAM
#define Z_FREE_VM_START	Z_BOOT_PHYS_TO_VIRT(Z_PHYS_RAM_END)
#else
#define Z_FREE_VM_START	Z_KERNEL_VIRT_END
#endif /* CONFIG_ARCH_MAPS_ALL_RAM */

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
 * This page frame has a clean copy in the backing store
 */
#define Z_PAGE_FRAME_BACKED		BIT(4)

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
		sys_snode_t node;
	};

	/* Z_PAGE_FRAME_* flags */
	uint8_t flags;

	/* TODO: Backing store and eviction algorithms may both need to
	 * introduce custom members for accounting purposes. Come up with
	 * a layer of abstraction for this. They may also want additional
	 * flags bits which shouldn't clobber each other. At all costs
	 * the total size of struct z_page_frame must be minimized.
	 */

	/* On Xtensa we can't pack this struct because of the memory alignment.
	 */
#ifdef CONFIG_XTENSA
} __aligned(4);
#else
} __packed;
#endif /* CONFIG_XTENSA */

static inline bool z_page_frame_is_pinned(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_PINNED) != 0U;
}

static inline bool z_page_frame_is_reserved(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_RESERVED) != 0U;
}

static inline bool z_page_frame_is_mapped(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_MAPPED) != 0U;
}

static inline bool z_page_frame_is_busy(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_BUSY) != 0U;
}

static inline bool z_page_frame_is_backed(struct z_page_frame *pf)
{
	return (pf->flags & Z_PAGE_FRAME_BACKED) != 0U;
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
	return page->flags == 0U;
}

static inline void z_assert_phys_aligned(uintptr_t phys)
{
	__ASSERT(phys % CONFIG_MMU_PAGE_SIZE == 0U,
		 "physical address 0x%lx is not page-aligned", phys);
	(void)phys;
}

extern struct z_page_frame z_page_frames[Z_NUM_PAGE_FRAMES];

static inline uintptr_t z_page_frame_to_phys(struct z_page_frame *pf)
{
	return (uintptr_t)((pf - z_page_frames) * CONFIG_MMU_PAGE_SIZE) +
			Z_PHYS_RAM_START;
}

/* Presumes there is but one mapping in the virtual address space */
static inline void *z_page_frame_to_virt(struct z_page_frame *pf)
{
	return pf->addr;
}

static inline bool z_is_page_frame(uintptr_t phys)
{
	z_assert_phys_aligned(phys);
	return IN_RANGE(phys, (uintptr_t)Z_PHYS_RAM_START,
			(uintptr_t)(Z_PHYS_RAM_END - 1));
}

static inline struct z_page_frame *z_phys_to_page_frame(uintptr_t phys)
{
	__ASSERT(z_is_page_frame(phys),
		 "0x%lx not an SRAM physical address", phys);

	return &z_page_frames[(phys - Z_PHYS_RAM_START) /
			      CONFIG_MMU_PAGE_SIZE];
}

static inline void z_mem_assert_virtual_region(uint8_t *addr, size_t size)
{
	__ASSERT((uintptr_t)addr % CONFIG_MMU_PAGE_SIZE == 0U,
		 "unaligned addr %p", addr);
	__ASSERT(size % CONFIG_MMU_PAGE_SIZE == 0U,
		 "unaligned size %zu", size);
	__ASSERT(!Z_DETECT_POINTER_OVERFLOW(addr, size),
		 "region %p size %zu zero or wraps around", addr, size);
	__ASSERT(IN_RANGE((uintptr_t)addr,
			  (uintptr_t)Z_VIRT_RAM_START,
			  ((uintptr_t)Z_VIRT_RAM_END - 1)) &&
		 IN_RANGE(((uintptr_t)addr + size - 1),
			  (uintptr_t)Z_VIRT_RAM_START,
			  ((uintptr_t)Z_VIRT_RAM_END - 1)),
		 "invalid virtual address region %p (%zu)", addr, size);
}

/* Debug function, pretty-print page frame information for all frames
 * concisely to printk.
 */
void z_page_frames_dump(void);

/* Convenience macro for iterating over all page frames */
#define Z_PAGE_FRAME_FOREACH(_phys, _pageframe) \
	for (_phys = Z_PHYS_RAM_START, _pageframe = z_page_frames; \
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
#endif /* CONFIG_DEMAND_PAGING */

#ifdef CONFIG_DEMAND_PAGING
/*
 * Core kernel demand paging APIs
 */

/**
 * Number of page faults since system startup
 *
 * Counts only those page faults that were handled successfully by the demand
 * paging mechanism and were not errors.
 *
 * @return Number of successful page faults
 */
unsigned long z_num_pagefaults_get(void);

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
 * @retval 0 Success
 * @retval -ENOMEM Insufficient backing store space
 */
int z_page_frame_evict(uintptr_t phys);

/**
 * Handle a page fault for a virtual data page
 *
 * This is invoked from the architecture page fault handler.
 *
 * If a valid page fault, the core kernel will obtain a page frame,
 * populate it with the data page that was evicted to the backing store,
 * update page tables, and return so that the faulting instruction may be
 * re-tried.
 *
 * The architecture must not call this function if the page was mapped and
 * not paged out at the time the exception was triggered (i.e. a protection
 * violation for a mapped page).
 *
 * If the faulting context had interrupts disabled when the page fault was
 * triggered, the entire page fault handling path must have interrupts
 * disabled, including the invocation of this function.
 *
 * Otherwise, interrupts may be enabled and the page fault handler may be
 * preemptible. Races to page-in will be appropriately handled by the kernel.
 *
 * @param addr Faulting virtual address
 * @retval true Page fault successfully handled, or nothing needed to be done.
 *              The arch layer should retry the faulting instruction.
 * @retval false This page fault was from an un-mapped page, should
 *               be treated as an error, and not re-tried.
 */
bool z_page_fault(void *addr);
#endif /* CONFIG_DEMAND_PAGING */
#endif /* CONFIG_MMU */
#endif /* KERNEL_INCLUDE_MMU_H */

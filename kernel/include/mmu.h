/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef KERNEL_INCLUDE_MMU_H
#define KERNEL_INCLUDE_MMU_H

#ifdef CONFIG_MMU

#include <stdint.h>
#include <sys/slist.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <sys/mem_manage.h>
#include <linker/linker-defs.h>

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
#define Z_NUM_PAGE_FRAMES	(Z_PHYS_RAM_SIZE / CONFIG_MMU_PAGE_SIZE)

/** End virtual address of virtual address space */
#define Z_VIRT_RAM_START	((uint8_t *)CONFIG_KERNEL_VM_BASE)
#define Z_VIRT_RAM_SIZE		((size_t)CONFIG_KERNEL_VM_SIZE)
#define Z_VIRT_RAM_END		(Z_VIRT_RAM_START + Z_VIRT_RAM_SIZE)

/* Boot-time virtual location of the kernel image. */
#define Z_KERNEL_VIRT_START	((uint8_t *)(&z_mapped_start))
#define Z_KERNEL_VIRT_END	((uint8_t *)(&z_mapped_end))
#define Z_KERNEL_VIRT_SIZE	((size_t)(&z_mapped_size))

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
} __packed;

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
	return page->flags == 0;
}

static inline void z_assert_phys_aligned(uintptr_t phys)
{
	__ASSERT(phys % CONFIG_MMU_PAGE_SIZE == 0,
		 "physical address 0x%lx is not page-aligned", phys);
	(void)phys;
}

/* Reserved pages */
#define Z_VM_RESERVED	0

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
	return (phys >= Z_PHYS_RAM_START) && (phys < Z_PHYS_RAM_END);
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
	__ASSERT((uintptr_t)addr % CONFIG_MMU_PAGE_SIZE == 0,
		 "unaligned addr %p", addr);
	__ASSERT(size % CONFIG_MMU_PAGE_SIZE == 0,
		 "unaligned size %zu", size);
	__ASSERT(addr + size > addr,
		 "region %p size %zu zero or wraps around", addr, size);
	__ASSERT(addr >= Z_VIRT_RAM_START && addr + size < Z_VIRT_RAM_END,
		 "invalid virtual address region %p (%zu)", addr, size);
}

/* Debug function, pretty-print page frame information for all frames
 * concisely to printk.
 */
void z_page_frames_dump(void);

/* Number of free page frames. This information may go stale immediately */
extern size_t z_free_page_count;

/* Convenience macro for iterating over all page frames */
#define Z_PAGE_FRAME_FOREACH(_phys, _pageframe) \
	for (_phys = Z_PHYS_RAM_START, _pageframe = z_page_frames; \
	     _phys < Z_PHYS_RAM_END; \
	     _phys += CONFIG_MMU_PAGE_SIZE, _pageframe++)

#endif /* CONFIG_MMU */
#endif /* KERNEL_INCLUDE_MMU_H */

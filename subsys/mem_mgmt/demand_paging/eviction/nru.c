/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Not Recently Used (NRU) eviction algorithm for demand paging
 */
#include <zephyr/kernel.h>
#include <mmu.h>
#include <kernel_arch_interface.h>
#include <zephyr/init.h>

#include <zephyr/kernel/mm/demand_paging.h>

/* The accessed and dirty states of each page frame are used to create
 * a hierarchy with a numerical value. When evicting a page, try to evict
 * page with the highest value (we prefer clean, not accessed pages).
 *
 * In this ontology, "accessed" means "recently accessed" and gets cleared
 * during the periodic update.
 *
 * 0 not accessed, clean
 * 1 not accessed, dirty
 * 2 accessed, clean
 * 3 accessed, dirty
 */
static void nru_periodic_update(struct k_timer *timer)
{
	uintptr_t phys;
	struct k_mem_page_frame *pf;
	unsigned int key = irq_lock();

	K_MEM_PAGE_FRAME_FOREACH(phys, pf) {
		if (!k_mem_page_frame_is_evictable(pf)) {
			continue;
		}

		/* Clear accessed bit in page tables */
		(void)arch_page_info_get(k_mem_page_frame_to_virt(pf),
					 NULL, true);
	}

	irq_unlock(key);
}

struct k_mem_page_frame *k_mem_paging_eviction_select(bool *dirty_ptr)
{
	unsigned int last_prec = 4U;
	struct k_mem_page_frame *last_pf = NULL, *pf;
	bool accessed;
	bool last_dirty = false;
	bool dirty = false;
	uintptr_t flags;
	uint32_t pf_idx;
	static uint32_t last_pf_idx;

	/* similar to K_MEM_PAGE_FRAME_FOREACH except we don't always start at 0 */
	last_pf_idx = (last_pf_idx + 1) % ARRAY_SIZE(k_mem_page_frames);
	pf_idx = last_pf_idx;
	do {
		pf = &k_mem_page_frames[pf_idx];
		pf_idx = (pf_idx + 1) % ARRAY_SIZE(k_mem_page_frames);

		unsigned int prec;

		if (!k_mem_page_frame_is_evictable(pf)) {
			continue;
		}

		flags = arch_page_info_get(k_mem_page_frame_to_virt(pf), NULL, false);
		accessed = (flags & ARCH_DATA_PAGE_ACCESSED) != 0UL;
		dirty = (flags & ARCH_DATA_PAGE_DIRTY) != 0UL;

		/* Implies a mismatch with page frame ontology and page
		 * tables
		 */
		__ASSERT((flags & ARCH_DATA_PAGE_LOADED) != 0U,
			 "non-present page, %s",
			 ((flags & ARCH_DATA_PAGE_NOT_MAPPED) != 0U) ?
			 "un-mapped" : "paged out");

		prec = (dirty ? 1U : 0U) + (accessed ? 2U : 0U);
		if (prec == 0) {
			/* If we find a not accessed, clean page we're done */
			last_pf = pf;
			last_dirty = dirty;
			break;
		}

		if (prec < last_prec) {
			last_prec = prec;
			last_pf = pf;
			last_dirty = dirty;
		}
	} while (pf_idx != last_pf_idx);

	/* Shouldn't ever happen unless every page is pinned */
	__ASSERT(last_pf != NULL, "no page to evict");

	last_pf_idx = last_pf - k_mem_page_frames;
	*dirty_ptr = last_dirty;

	return last_pf;
}

static K_TIMER_DEFINE(nru_timer, nru_periodic_update, NULL);

void k_mem_paging_eviction_init(void)
{
	k_timer_start(&nru_timer, K_NO_WAIT,
		      K_MSEC(CONFIG_EVICTION_NRU_PERIOD));
}

#ifdef CONFIG_EVICTION_TRACKING
/*
 * Empty functions defined here so that architectures unconditionally
 * implement eviction tracking can still use this algorithm for
 * testing.
 */

void k_mem_paging_eviction_add(struct k_mem_page_frame *pf)
{
	ARG_UNUSED(pf);
}

void k_mem_paging_eviction_remove(struct k_mem_page_frame *pf)
{
	ARG_UNUSED(pf);
}

void k_mem_paging_eviction_accessed(uintptr_t phys)
{
	ARG_UNUSED(phys);
}

#endif /* CONFIG_EVICTION_TRACKING */

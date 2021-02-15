/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Not Recently Used (NRU) eviction algorithm for demand paging
 */
#include <kernel.h>
#include <mmu.h>
#include <kernel_arch_interface.h>
#include <init.h>

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
	struct z_page_frame *pf;
	int key = irq_lock();

	Z_PAGE_FRAME_FOREACH(phys, pf) {
		if (!z_page_frame_is_evictable(pf)) {
			continue;
		}

		/* Clear accessed bit in page tables */
		(void)arch_page_info_get(pf->addr, NULL, true);
	}

	irq_unlock(key);
}

struct z_page_frame *z_eviction_select(bool *dirty_ptr)
{
	unsigned int last_prec = 4U;
	struct z_page_frame *last_pf = NULL, *pf;
	bool accessed;
	bool dirty = false;
	uintptr_t flags, phys;

	Z_PAGE_FRAME_FOREACH(phys, pf) {
		unsigned int prec;

		if (!z_page_frame_is_evictable(pf)) {
			continue;
		}

		flags = arch_page_info_get(pf->addr, NULL, false);
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
			break;
		}

		if (prec < last_prec) {
			last_prec = prec;
			last_pf = pf;
		}
	}
	/* Shouldn't ever happen unless every page is pinned */
	__ASSERT(last_pf != NULL, "no page to evict");

	*dirty_ptr = dirty;

	return last_pf;
}

static K_TIMER_DEFINE(nru_timer, nru_periodic_update, NULL);

void z_eviction_init(void)
{
	k_timer_start(&nru_timer, K_NO_WAIT,
		      K_MSEC(CONFIG_EVICTION_NRU_PERIOD));
}

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RAM-based memory buffer backing store implementation for demo purposes
 */
#include <mmu.h>
#include <string.h>
#include <kernel_arch_interface.h>
#include <zephyr/kernel/mm/demand_paging.h>

/*
 * TODO:
 *
 * This is a demonstration backing store for testing the kernel side of the
 * demand paging feature. In production there are basically two types of
 * backing stores:
 *
 * 1) A large, sparse backing store that is big enough to capture the entire
 *    address space. Implementation of these is very simple; the location
 *    token is just a function of the evicted virtual address and no space
 *    management is necessary. Clean copies of paged-in data pages may be kept
 *    indefinitely.
 *
 * 2) A backing store that has limited storage space, and is not sufficiently
 *    large to hold clean copies of all mapped memory.
 *
 * This backing store is an example of the latter case. However, locations
 * are freed as soon as pages are paged in, in
 * k_mem_paging_backing_store_page_finalize().
 * This implies that all data pages are treated as dirty as
 * K_MEM_PAGE_FRAME_BACKED is never set, even if the data page was paged out before
 * and not modified since then.
 *
 * An optimization a real backing store will want is have
 * k_mem_paging_backing_store_page_finalize() note the storage location of
 * a paged-in data page in a custom field of its associated k_mem_page_frame, and
 * set the K_MEM_PAGE_FRAME_BACKED bit. Invocations of
 * k_mem_paging_backing_store_location_get() will have logic to return
 * the previous clean page location instead of allocating
 * a new one if K_MEM_PAGE_FRAME_BACKED is set.
 *
 * This will, however, require the implementation of a clean page
 * eviction algorithm, to free backing store locations for loaded data pages
 * as the backing store fills up, and clear the K_MEM_PAGE_FRAME_BACKED bit
 * appropriately.
 *
 * All of this logic is local to the backing store implementation; from the
 * core kernel's perspective the only change is that K_MEM_PAGE_FRAME_BACKED
 * starts getting set for certain page frames after a page-in (and possibly
 * cleared at a later time).
 */
#define BACKING_STORE_SIZE (CONFIG_BACKING_STORE_RAM_PAGES * CONFIG_MMU_PAGE_SIZE)
static char backing_store[BACKING_STORE_SIZE] __aligned(sizeof(void *));
static struct k_mem_slab backing_slabs;
static unsigned int free_slabs;

static void *location_to_slab(uintptr_t location)
{
	__ASSERT(location % CONFIG_MMU_PAGE_SIZE == 0,
		 "unaligned location 0x%lx", location);
	__ASSERT(location <
		 (CONFIG_BACKING_STORE_RAM_PAGES * CONFIG_MMU_PAGE_SIZE),
		 "bad location 0x%lx, past bounds of backing store", location);

	return backing_store + location;
}

static uintptr_t slab_to_location(void *slab)
{
	char *pos = slab;
	uintptr_t offset;

	__ASSERT(pos >= backing_store &&
		 pos < backing_store + ARRAY_SIZE(backing_store),
		 "bad slab pointer %p", slab);
	offset = pos - backing_store;
	__ASSERT(offset % CONFIG_MMU_PAGE_SIZE == 0,
		 "unaligned slab pointer %p", slab);

	return offset;
}

int k_mem_paging_backing_store_location_get(struct k_mem_page_frame *pf,
					    uintptr_t *location,
					    bool page_fault)
{
	int ret;
	void *slab;

	if ((!page_fault && free_slabs == 1) || free_slabs == 0) {
		return -ENOMEM;
	}

	ret = k_mem_slab_alloc(&backing_slabs, &slab, K_NO_WAIT);
	__ASSERT(ret == 0, "slab count mismatch");
	if (ret != 0) {
		return ret;
	}
	*location = slab_to_location(slab);
	free_slabs--;

	return 0;
}

void k_mem_paging_backing_store_location_free(uintptr_t location)
{
	void *slab = location_to_slab(location);

	k_mem_slab_free(&backing_slabs, slab);
	free_slabs++;
}

void k_mem_paging_backing_store_page_out(uintptr_t location)
{
	(void)memcpy(location_to_slab(location), K_MEM_SCRATCH_PAGE,
		     CONFIG_MMU_PAGE_SIZE);
}

void k_mem_paging_backing_store_page_in(uintptr_t location)
{
	(void)memcpy(K_MEM_SCRATCH_PAGE, location_to_slab(location),
		     CONFIG_MMU_PAGE_SIZE);
}

void k_mem_paging_backing_store_page_finalize(struct k_mem_page_frame *pf,
					      uintptr_t location)
{
#ifdef CONFIG_DEMAND_MAPPING
	/* ignore those */
	if (location == ARCH_UNPAGED_ANON_ZERO || location == ARCH_UNPAGED_ANON_UNINIT) {
		return;
	}
#endif
	k_mem_paging_backing_store_location_free(location);
}

void k_mem_paging_backing_store_init(void)
{
	k_mem_slab_init(&backing_slabs, backing_store, CONFIG_MMU_PAGE_SIZE,
			CONFIG_BACKING_STORE_RAM_PAGES);
	free_slabs = CONFIG_BACKING_STORE_RAM_PAGES;
}

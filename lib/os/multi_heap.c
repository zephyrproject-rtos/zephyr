/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/__assert.h>
#include <sys/util.h>
#include <sys/sys_heap.h>
#include <sys/multi_heap.h>

void sys_multi_heap_init(struct sys_multi_heap *heap, sys_multi_heap_fn_t choice_fn)
{
	heap->nheaps = 0;
	heap->choice = choice_fn;
}

void sys_multi_heap_add_heap(struct sys_multi_heap *mheap, struct sys_heap *heap)
{
	__ASSERT_NO_MSG(mheap->nheaps < ARRAY_SIZE(mheap->heaps));

	mheap->heaps[mheap->nheaps++] = heap;

	/* Now sort them in memory order, simple extraction sort */
	for (int i = 0; i < mheap->nheaps; i++) {
		void *swap;
		int lowest = -1;
		uintptr_t lowest_addr = UINTPTR_MAX;

		for (int j = i; j < mheap->nheaps; j++) {
			uintptr_t haddr = (uintptr_t)mheap->heaps[j]->heap;

			if (haddr < lowest_addr) {
				lowest = j;
				lowest_addr = haddr;
			}
		}
		swap = mheap->heaps[i];
		mheap->heaps[i] = mheap->heaps[lowest];
		mheap->heaps[lowest] = swap;
	}
}

void *sys_multi_heap_alloc(struct sys_multi_heap *mheap, void *cfg, size_t bytes)
{
	return mheap->choice(mheap, cfg, 0, bytes);
}

void *sys_multi_heap_aligned_alloc(struct sys_multi_heap *mheap,
				   void *cfg, size_t align, size_t bytes)
{
	return mheap->choice(mheap, cfg, align, bytes);
}

void sys_multi_heap_free(struct sys_multi_heap *mheap, void *block)
{
	uintptr_t haddr, baddr = (uintptr_t) block;
	int i;

	/* Search the heaps array to find the correct heap
	 *
	 * FIXME: just a linear search currently, as the list is
	 * always short for reasonable apps and this code is very
	 * quick.  The array is stored in sorted order though, so a
	 * binary search based on the block address is the design
	 * goal.
	 */
	for (i = 0; i < mheap->nheaps; i++) {
		haddr = (uintptr_t)mheap->heaps[i]->heap;
		if (baddr < haddr) {
			break;
		}
	}

	/* Now i stores the index of the heap after our target (even
	 * if it's invalid and our target is the last!)
	 */
	sys_heap_free(mheap->heaps[i-1], block);
}

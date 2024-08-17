/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/multi_heap.h>
#include <string.h>

void sys_multi_heap_init(struct sys_multi_heap *heap, sys_multi_heap_fn_t choice_fn)
{
	heap->nheaps = 0;
	heap->choice = choice_fn;
}

void sys_multi_heap_add_heap(struct sys_multi_heap *mheap,
			struct sys_heap *heap, void *user_data)
{
	__ASSERT_NO_MSG(mheap->nheaps < ARRAY_SIZE(mheap->heaps));

	mheap->heaps[mheap->nheaps].heap = heap;
	mheap->heaps[mheap->nheaps++].user_data = user_data;

	/* Now sort them in memory order, simple extraction sort */
	for (int i = 0; i < mheap->nheaps; i++) {
		struct sys_multi_heap_rec swap;
		int lowest = -1;
		uintptr_t lowest_addr = UINTPTR_MAX;

		for (int j = i; j < mheap->nheaps; j++) {
			uintptr_t haddr = (uintptr_t)mheap->heaps[j].heap->heap;

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

const struct sys_multi_heap_rec *sys_multi_heap_get_heap(const struct sys_multi_heap *mheap,
							 void *addr)
{
	uintptr_t haddr, baddr = (uintptr_t) addr;
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
		haddr = (uintptr_t)mheap->heaps[i].heap->heap;
		if (baddr < haddr) {
			break;
		}
	}

	/* Now i stores the index of the heap after our target (even
	 * if it's invalid and our target is the last!)
	 * FIXME: return -ENOENT when a proper heap is not found
	 */
	return &mheap->heaps[i-1];
}


void sys_multi_heap_free(struct sys_multi_heap *mheap, void *block)
{
	const struct sys_multi_heap_rec *heap;

	heap = sys_multi_heap_get_heap(mheap, block);

	if (heap != NULL) {
		sys_heap_free(heap->heap, block);
	}
}

void *sys_multi_heap_aligned_realloc(struct sys_multi_heap *mheap, void *cfg,
				     void *ptr, size_t align, size_t bytes)
{
	/* special realloc semantics */
	if (ptr == NULL) {
		return sys_multi_heap_aligned_alloc(mheap, cfg, align, bytes);
	}
	if (bytes == 0) {
		sys_multi_heap_free(mheap, ptr);
		return NULL;
	}

	const struct sys_multi_heap_rec *rec = sys_multi_heap_get_heap(mheap, ptr);

	__ASSERT_NO_MSG(rec);

	/* Invoke the realloc function on the same heap, to try to reuse in place */
	void *new_ptr = sys_heap_aligned_realloc(rec->heap, ptr, align, bytes);

	if (new_ptr != NULL) {
		return new_ptr;
	}

	size_t old_size = sys_heap_usable_size(rec->heap, ptr);

	/* Otherwise, allocate a new block and copy the data */
	new_ptr = sys_multi_heap_aligned_alloc(mheap, cfg, align, bytes);
	if (new_ptr != NULL) {
		memcpy(new_ptr, ptr, MIN(old_size, bytes));
		sys_multi_heap_free(mheap, ptr);
	}

	return new_ptr;
}

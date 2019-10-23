/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/sys_heap.h>
#include <kernel.h>
#include "heap.h"

static void *chunk_mem(struct z_heap *h, chunkid_t c)
{
	chunk_unit_t *buf = chunk_buf(h);
	uint8_t *ret = ((uint8_t *)&buf[c]) + chunk_header_bytes(h);

	CHECK(!(((size_t)ret) & (big_heap(h) ? 7 : 3)));

	return ret;
}

static void free_list_remove(struct z_heap *h, int bidx,
			     chunkid_t c)
{
	struct z_heap_bucket *b = &h->buckets[bidx];

	CHECK(!chunk_used(h, c));
	CHECK(b->next != 0);
	CHECK(h->avail_buckets & (1 << bidx));

	if (next_free_chunk(h, c) == c) {
		/* this is the last chunk */
		h->avail_buckets &= ~(1 << bidx);
		b->next = 0;
	} else {
		chunkid_t first = prev_free_chunk(h, c),
			  second = next_free_chunk(h, c);

		b->next = second;
		set_next_free_chunk(h, first, second);
		set_prev_free_chunk(h, second, first);
	}
}

static void free_list_add(struct z_heap *h, chunkid_t c)
{
	int bi = bucket_idx(h, chunk_size(h, c));

	if (h->buckets[bi].next == 0) {
		CHECK((h->avail_buckets & (1 << bi)) == 0);

		/* Empty list, first item */
		h->avail_buckets |= (1 << bi);
		h->buckets[bi].next = c;
		set_prev_free_chunk(h, c, c);
		set_next_free_chunk(h, c, c);
	} else {
		CHECK(h->avail_buckets & (1 << bi));

		/* Insert before (!) the "next" pointer */
		chunkid_t second = h->buckets[bi].next;
		chunkid_t first = prev_free_chunk(h, second);

		set_prev_free_chunk(h, c, first);
		set_next_free_chunk(h, c, second);
		set_next_free_chunk(h, first, c);
		set_prev_free_chunk(h, second, c);
	}
}

/* Allocates (fit check has already been perfomred) from the next
 * chunk at the specified bucket level
 */
static void *split_alloc(struct z_heap *h, int bidx, size_t sz)
{
	CHECK(h->buckets[bidx].next != 0
	      && sz <= chunk_size(h, h->buckets[bidx].next));

	chunkid_t c = h->buckets[bidx].next;

	free_list_remove(h, bidx, c);

	/* Split off remainder if it's usefully large */
	size_t rem = chunk_size(h, c) - sz;

	CHECK(rem < h->len);

	if (rem >= min_chunk_size(h)) {
		chunkid_t c2 = c + sz;
		chunkid_t c3 = right_chunk(h, c);

		set_chunk_size(h, c, sz);
		set_chunk_size(h, c2, rem);
		set_left_chunk_size(h, c2, sz);
		set_left_chunk_size(h, c3, rem);
		free_list_add(h, c2);
	}

	set_chunk_used(h, c, true);

	return chunk_mem(h, c);
}

void sys_heap_free(struct sys_heap *heap, void *mem)
{
	if (mem == NULL) {
		return; /* ISO C free() semantics */
	}

	struct z_heap *h = heap->heap;
	chunkid_t c = ((uint8_t *)mem - chunk_header_bytes(h)
		       - (uint8_t *)chunk_buf(h)) / CHUNK_UNIT;

	/*
	 * This should catch many double-free cases.
	 * This is cheap enough so let's do it all the time.
	 */
	__ASSERT(chunk_used(h, c),
		 "unexpected heap state (double-free?) for memory at %p", mem);
	/*
	 * It is easy to catch many common memory overflow cases with
	 * a quick check on this and next chunk header fields that are
	 * immediately before and after the freed memory.
	 */
	__ASSERT(left_chunk(h, right_chunk(h, c)) == c,
		 "corrupted heap bounds (buffer overflow?) for memory at %p", mem);

	/* Merge with right chunk?  We can just absorb it. */
	if (!chunk_used(h, right_chunk(h, c))) {
		chunkid_t rc = right_chunk(h, c);
		size_t newsz = chunk_size(h, c) + chunk_size(h, rc);

		free_list_remove(h, bucket_idx(h, chunk_size(h, rc)), rc);
		set_chunk_size(h, c, newsz);
		set_left_chunk_size(h, right_chunk(h, c), newsz);
	}

	/* Merge with left chunk?  It absorbs us. */
	if (!chunk_used(h, left_chunk(h, c))) {
		chunkid_t lc = left_chunk(h, c);
		chunkid_t rc = right_chunk(h, c);
		size_t csz = chunk_size(h, c);
		size_t merged_sz = csz + chunk_size(h, lc);

		free_list_remove(h, bucket_idx(h, chunk_size(h, lc)), lc);
		set_chunk_size(h, lc, merged_sz);
		set_left_chunk_size(h, rc, merged_sz);

		c = lc;
	}

	set_chunk_used(h, c, false);
	free_list_add(h, c);
}

void *sys_heap_alloc(struct sys_heap *heap, size_t bytes)
{
	struct z_heap *h = heap->heap;
	size_t sz = bytes_to_chunksz(h, bytes);
	int bi = bucket_idx(h, sz);
	struct z_heap_bucket *b = &h->buckets[bi];

	if (bytes == 0 || bi > bucket_idx(h, h->len)) {
		return NULL;
	}

	/* First try a bounded count of items from the minimal bucket
	 * size.  These may not fit, trying (e.g.) three means that
	 * (assuming that chunk sizes are evenly distributed[1]) we
	 * have a 7/8 chance of finding a match, thus keeping the
	 * number of such blocks consumed by allocation higher than
	 * the number of smaller blocks created by fragmenting larger
	 * ones.
	 *
	 * [1] In practice, they are never evenly distributed, of
	 * course.  But even in pathological situations we still
	 * maintain our constant time performance and at worst see
	 * fragmentation waste of the order of the block allocated
	 * only.
	 */
	if (b->next) {
		chunkid_t first = b->next;
		int i = CONFIG_SYS_HEAP_ALLOC_LOOPS;
		do {
			if (chunk_size(h, b->next) >= sz) {
				return split_alloc(h, bi, sz);
			}
			b->next = next_free_chunk(h, b->next);
			CHECK(b->next != 0);
		} while (--i && b->next != first);
	}

	/* Otherwise pick the smallest non-empty bucket guaranteed to
	 * fit and use that unconditionally.
	 */
	size_t bmask = h->avail_buckets & ~((1 << (bi + 1)) - 1);

	if ((bmask & h->avail_buckets) != 0) {
		int minbucket = __builtin_ctz(bmask & h->avail_buckets);

		return split_alloc(h, minbucket, sz);
	}

	return NULL;
}

void sys_heap_init(struct sys_heap *heap, void *mem, size_t bytes)
{
	/* Must fit in a 32 bit count of HUNK_UNIT */
	CHECK(bytes / CHUNK_UNIT <= 0xffffffffU);

	/* Reserve the final marker chunk's header */
	CHECK(bytes > heap_footer_bytes(bytes));
	bytes -= heap_footer_bytes(bytes);

	/* Round the start up, the end down */
	uintptr_t addr = ROUND_UP(mem, CHUNK_UNIT);
	uintptr_t end = ROUND_DOWN((uint8_t *)mem + bytes, CHUNK_UNIT);
	size_t buf_sz = (end - addr) / CHUNK_UNIT;

	CHECK(end > addr);
	CHECK(buf_sz > chunksz(sizeof(struct z_heap)));

	struct z_heap *h = (struct z_heap *)addr;
	heap->heap = h;
	h->chunk0_hdr_area = 0;
	h->len = buf_sz;
	h->avail_buckets = 0;

	int nb_buckets = bucket_idx(h, buf_sz) + 1;
	size_t chunk0_size = chunksz(sizeof(struct z_heap) +
				     nb_buckets * sizeof(struct z_heap_bucket));

	CHECK(chunk0_size + min_chunk_size(h) < buf_sz);

	for (int i = 0; i < nb_buckets; i++) {
		h->buckets[i].next = 0;
	}

	/* chunk containing our struct z_heap */
	set_chunk_size(h, 0, chunk0_size);
	set_chunk_used(h, 0, true);

	/* chunk containing the free heap */
	set_chunk_size(h, chunk0_size, buf_sz - chunk0_size);
	set_left_chunk_size(h, chunk0_size, chunk0_size);

	/* the end marker chunk */
	set_chunk_size(h, buf_sz, 0);
	set_left_chunk_size(h, buf_sz, buf_sz - chunk0_size);
	set_chunk_used(h, buf_sz, true);

	free_list_add(h, chunk0_size);
}

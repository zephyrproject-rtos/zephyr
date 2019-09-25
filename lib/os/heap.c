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
	uint8_t *ret = ((uint8_t *)&h->buf[c]) + chunk_header_bytes(h);

	CHECK(!(((size_t)ret) & (big_heap(h) ? 7 : 3)));

	return ret;
}

static void free_list_remove(struct z_heap *h, int bidx,
			     chunkid_t c)
{
	struct z_heap_bucket *b = &h->buckets[bidx];

	CHECK(!chunk_used(h, c));
	CHECK(b->next != 0);
	CHECK(b->list_size > 0);
	CHECK((((h->avail_buckets & (1 << bidx)) == 0)
	       == (h->buckets[bidx].next == 0)));

	b->list_size--;

	if (b->list_size == 0) {
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

	if (h->buckets[bi].list_size++ == 0) {
		CHECK(h->buckets[bi].next == 0);
		CHECK((h->avail_buckets & (1 << bi)) == 0);

		/* Empty list, first item */
		h->avail_buckets |= (1 << bi);
		h->buckets[bi].next = c;
		set_prev_free_chunk(h, c, c);
		set_next_free_chunk(h, c, c);
	} else {
		/* Insert before (!) the "next" pointer */
		chunkid_t second = h->buckets[bi].next;
		chunkid_t first = prev_free_chunk(h, second);

		set_prev_free_chunk(h, c, first);
		set_next_free_chunk(h, c, second);
		set_next_free_chunk(h, first, c);
		set_prev_free_chunk(h, second, c);
	}

	CHECK(h->avail_buckets & (1 << bucket_idx(h, chunk_size(h, c))));
}

static ALWAYS_INLINE bool last_chunk(struct z_heap *h, chunkid_t c)
{
	return (c + chunk_size(h, c)) == h->len;
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

	if (rem >= (big_heap(h) ? 2 : 1)) {
		chunkid_t c2 = c + sz;
		chunkid_t c3 = right_chunk(h, c);

		set_chunk_size(h, c, sz);
		set_chunk_size(h, c2, rem);
		set_left_chunk_size(h, c2, sz);
		if (!last_chunk(h, c2)) {
			set_left_chunk_size(h, c3, rem);
		}
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
		       - (uint8_t *)h->buf) / CHUNK_UNIT;

	/* Merge with right chunk?  We can just absorb it. */
	if (!last_chunk(h, c) && !chunk_used(h, right_chunk(h, c))) {
		chunkid_t rc = right_chunk(h, c);
		size_t newsz = chunk_size(h, c) + chunk_size(h, rc);

		free_list_remove(h, bucket_idx(h, chunk_size(h, rc)), rc);
		set_chunk_size(h, c, newsz);
		if (!last_chunk(h, c)) {
			set_left_chunk_size(h, right_chunk(h, c), newsz);
		}
	}

	/* Merge with left chunk?  It absorbs us. */
	if (c != h->chunk0 && !chunk_used(h, left_chunk(h, c))) {
		chunkid_t lc = left_chunk(h, c);
		chunkid_t rc = right_chunk(h, c);
		size_t csz = chunk_size(h, c);
		size_t merged_sz = csz + chunk_size(h, lc);

		free_list_remove(h, bucket_idx(h, chunk_size(h, lc)), lc);
		set_chunk_size(h, lc, merged_sz);
		if (!last_chunk(h, lc)) {
			set_left_chunk_size(h, rc, merged_sz);
		}

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
	int loops = MIN(b->list_size, CONFIG_SYS_HEAP_ALLOC_LOOPS);

	for (int i = 0; i < loops; i++) {
		CHECK(b->next != 0);
		if (chunk_size(h, b->next) >= sz) {
			return split_alloc(h, bi, sz);
		}
		b->next = next_free_chunk(h, b->next);
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
	/* Must fit in a 32 bit count of u64's */
#if __SIZEOF_SIZE_T__ > 4
	CHECK(bytes < 0x800000000ULL);
#endif

	/* Round the start up, the end down */
	size_t addr = ((size_t)mem + CHUNK_UNIT - 1) & ~(CHUNK_UNIT - 1);
	size_t end = ((size_t)mem + bytes) & ~(CHUNK_UNIT - 1);
	size_t buf_sz = (end - addr) / CHUNK_UNIT;
	size_t hdr_chunks = chunksz(sizeof(struct z_heap));

	CHECK(end > addr);

	struct z_heap *h = (struct z_heap *)addr;

	heap->heap = (struct z_heap *)addr;
	h->buf = (uint64_t *)addr;
	h->buckets = (void *)(addr + CHUNK_UNIT * hdr_chunks);
	h->len = buf_sz;
	h->avail_buckets = 0;

	size_t buckets_bytes = ((bucket_idx(h, buf_sz) + 1)
				* sizeof(struct z_heap_bucket));

	h->chunk0 = hdr_chunks + chunksz(buckets_bytes);

	for (int i = 0; i <= bucket_idx(heap->heap, heap->heap->len); i++) {
		heap->heap->buckets[i].list_size = 0;
		heap->heap->buckets[i].next = 0;
	}

	set_chunk_size(h, h->chunk0, buf_sz - h->chunk0);
	free_list_add(h, h->chunk0);
}

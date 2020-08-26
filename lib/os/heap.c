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

static void free_list_remove_bidx(struct z_heap *h, chunkid_t c, int bidx)
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

static void free_list_remove(struct z_heap *h, chunkid_t c)
{
	if (!solo_free_header(h, c)) {
		int bidx = bucket_idx(h, chunk_size(h, c));
		free_list_remove_bidx(h, c, bidx);
	}
}

static void free_list_add_bidx(struct z_heap *h, chunkid_t c, int bidx)
{
	struct z_heap_bucket *b = &h->buckets[bidx];

	if (b->next == 0) {
		CHECK((h->avail_buckets & (1 << bidx)) == 0);

		/* Empty list, first item */
		h->avail_buckets |= (1 << bidx);
		b->next = c;
		set_prev_free_chunk(h, c, c);
		set_next_free_chunk(h, c, c);
	} else {
		CHECK(h->avail_buckets & (1 << bidx));

		/* Insert before (!) the "next" pointer */
		chunkid_t second = b->next;
		chunkid_t first = prev_free_chunk(h, second);

		set_prev_free_chunk(h, c, first);
		set_next_free_chunk(h, c, second);
		set_next_free_chunk(h, first, c);
		set_prev_free_chunk(h, second, c);
	}
}

static void free_list_add(struct z_heap *h, chunkid_t c)
{
	if (!solo_free_header(h, c)) {
		int bidx = bucket_idx(h, chunk_size(h, c));
		free_list_add_bidx(h, c, bidx);
	}
}

/* Splits a chunk "lc" into a left chunk and a right chunk at "rc".
 * Leaves both chunks marked "free"
 */
static void split_chunks(struct z_heap *h, chunkid_t lc, chunkid_t rc)
{
	CHECK(rc > lc);
	CHECK(rc - lc < chunk_size(h, lc));

	size_t sz0 = chunk_size(h, lc);
	size_t lsz = rc - lc;
	size_t rsz = sz0 - lsz;

	set_chunk_size(h, lc, lsz);
	set_chunk_size(h, rc, rsz);
	set_left_chunk_size(h, rc, lsz);
	set_left_chunk_size(h, right_chunk(h, rc), rsz);
}

/* Does not modify free list */
static void merge_chunks(struct z_heap *h, chunkid_t lc, chunkid_t rc)
{
	size_t newsz = chunk_size(h, lc) + chunk_size(h, rc);

	set_chunk_size(h, lc, newsz);
	set_left_chunk_size(h, right_chunk(h, rc), newsz);
}

static void free_chunk(struct z_heap *h, chunkid_t c)
{
	/* Merge with free right chunk? */
	if (!chunk_used(h, right_chunk(h, c))) {
		free_list_remove(h, right_chunk(h, c));
		merge_chunks(h, c, right_chunk(h, c));
	}

	/* Merge with free left chunk? */
	if (!chunk_used(h, left_chunk(h, c))) {
		free_list_remove(h, left_chunk(h, c));
		merge_chunks(h, left_chunk(h, c), c);
		c = left_chunk(h, c);
	}

	free_list_add(h, c);
}

/*
 * Return the closest chunk ID corresponding to given memory pointer.
 * Here "closest" is only meaningful in the context of sys_heap_aligned_alloc()
 * where wanted alignment might not always correspond to a chunk header
 * boundary.
 */
static chunkid_t mem_to_chunkid(struct z_heap *h, void *p)
{
	uint8_t *mem = p, *base = (uint8_t *)chunk_buf(h);
	return (mem - chunk_header_bytes(h) - base) / CHUNK_UNIT;
}

void sys_heap_free(struct sys_heap *heap, void *mem)
{
	if (mem == NULL) {
		return; /* ISO C free() semantics */
	}
	struct z_heap *h = heap->heap;
	chunkid_t c = mem_to_chunkid(h, mem);

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
		 "corrupted heap bounds (buffer overflow?) for memory at %p",
		 mem);

	set_chunk_used(h, c, false);
	free_chunk(h, c);
}

static chunkid_t alloc_chunk(struct z_heap *h, size_t sz)
{
	int bi = bucket_idx(h, sz);
	struct z_heap_bucket *b = &h->buckets[bi];

	if (bi > bucket_idx(h, h->len)) {
		return 0;
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
			chunkid_t c = b->next;
			if (chunk_size(h, c) >= sz) {
				free_list_remove_bidx(h, c, bi);
				return c;
			}
			b->next = next_free_chunk(h, c);
			CHECK(b->next != 0);
		} while (--i && b->next != first);
	}

	/* Otherwise pick the smallest non-empty bucket guaranteed to
	 * fit and use that unconditionally.
	 */
	size_t bmask = h->avail_buckets & ~((1 << (bi + 1)) - 1);

	if ((bmask & h->avail_buckets) != 0) {
		int minbucket = __builtin_ctz(bmask & h->avail_buckets);
		chunkid_t c = h->buckets[minbucket].next;

		free_list_remove_bidx(h, c, minbucket);
		CHECK(chunk_size(h, c) >= sz);
		return c;
	}

	return 0;
}

void *sys_heap_alloc(struct sys_heap *heap, size_t bytes)
{
	if (bytes == 0) {
		return NULL;
	}

	struct z_heap *h = heap->heap;
	size_t chunk_sz = bytes_to_chunksz(h, bytes);
	chunkid_t c = alloc_chunk(h, chunk_sz);
	if (c == 0) {
		return NULL;
	}

	/* Split off remainder if any */
	if (chunk_size(h, c) > chunk_sz) {
		split_chunks(h, c, c + chunk_sz);
		free_list_add(h, c + chunk_sz);
	}

	set_chunk_used(h, c, true);
	return chunk_mem(h, c);
}

void *sys_heap_aligned_alloc(struct sys_heap *heap, size_t align, size_t bytes)
{
	struct z_heap *h = heap->heap;

	CHECK((align & (align - 1)) == 0);

	if (align <= chunk_header_bytes(h)) {
		return sys_heap_alloc(heap, bytes);
	}
	if (bytes == 0) {
		return NULL;
	}

	/*
	 * Find a free block that is guaranteed to fit.
	 * We over-allocate to account for alignment and then free
	 * the extra allocations afterwards.
	 */
	size_t alloc_sz = bytes_to_chunksz(h, bytes);
	size_t padded_sz = bytes_to_chunksz(h, bytes + align - 1);
	chunkid_t c0 = alloc_chunk(h, padded_sz);

	if (c0 == 0) {
		return NULL;
	}

	/* Align allocated memory */
	void *mem = chunk_mem(h, c0);
	mem = (void *) ROUND_UP(mem, align);

	/* Get corresponding chunk */
	chunkid_t c = mem_to_chunkid(h, mem);
	CHECK(c >= c0 && c  < c0 + padded_sz);

	/* Split and free unused prefix */
	if (c > c0) {
		split_chunks(h, c0, c);
		free_list_add(h, c0);
	}

	/* Split and free unused suffix */
	if (chunk_size(h, c) > alloc_sz) {
		split_chunks(h, c, c + alloc_sz);
		free_list_add(h, c + alloc_sz);
	}

	set_chunk_used(h, c, true);
	return mem;
}

void sys_heap_init(struct sys_heap *heap, void *mem, size_t bytes)
{
	/* Must fit in a 32 bit count of HUNK_UNIT */
	__ASSERT(bytes / CHUNK_UNIT <= 0xffffffffU, "heap size is too big");

	/* Reserve the final marker chunk's header */
	__ASSERT(bytes > heap_footer_bytes(bytes), "heap size is too small");
	bytes -= heap_footer_bytes(bytes);

	/* Round the start up, the end down */
	uintptr_t addr = ROUND_UP(mem, CHUNK_UNIT);
	uintptr_t end = ROUND_DOWN((uint8_t *)mem + bytes, CHUNK_UNIT);
	size_t buf_sz = (end - addr) / CHUNK_UNIT;

	CHECK(end > addr);
	__ASSERT(buf_sz > chunksz(sizeof(struct z_heap)), "heap size is too small");

	struct z_heap *h = (struct z_heap *)addr;
	heap->heap = h;
	h->chunk0_hdr_area = 0;
	h->len = buf_sz;
	h->avail_buckets = 0;

	int nb_buckets = bucket_idx(h, buf_sz) + 1;
	size_t chunk0_size = chunksz(sizeof(struct z_heap) +
				     nb_buckets * sizeof(struct z_heap_bucket));

	__ASSERT(chunk0_size + min_chunk_size(h) < buf_sz, "heap size is too small");

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

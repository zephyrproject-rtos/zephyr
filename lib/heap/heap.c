/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/kernel.h>
#include <string.h>
#include "heap.h"
#ifdef CONFIG_MSAN
#include <sanitizer/msan_interface.h>
#endif
#ifdef CONFIG_SYS_HEAP_CANARIES
#include <zephyr/random/random.h>
#endif

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
static inline void increase_allocated_bytes(struct z_heap *h, size_t num_bytes)
{
	h->allocated_bytes += num_bytes;
	h->max_allocated_bytes = max(h->max_allocated_bytes, h->allocated_bytes);
}
#endif

static void *chunk_mem(struct z_heap *h, chunkid_t c)
{
	chunk_unit_t *buf = chunk_buf(h);
	uint8_t *ret = NULL;

#ifdef CONFIG_SYS_HEAP_CANARIES
	/* Special chunks (0 and end_chunk) do not have a custom header.
	 * For other chunks, the user data pointer is offset by the total chunk header size.
	 */
	if (c != 0 && c != h->end_chunk) {
		ret = ((uint8_t *)&buf[c]) + chunk_header_bytes(h);
	} else {
		/* For chunk 0 or end_chunk, only Zephyr's base internal header applies,
		 * as these are special internal chunks that do not hold user data.
		 */
		ret = ((uint8_t *)&buf[c]) + zephyr_base_header_bytes(h);
	}
#else
	/* If custom header is not enabled, use the standard chunk_header_bytes */
	ret = ((uint8_t *)&buf[c]) + chunk_header_bytes(h);
#endif

	CHECK(!(((uintptr_t)ret) & (big_heap(h) ? 7 : 3)));

	return ret;
}

static void free_list_remove_bidx(struct z_heap *h, chunkid_t c, int bidx)
{
	struct z_heap_bucket *b = &h->buckets[bidx];

	CHECK(!chunk_used(h, c));
	CHECK(b->next != 0);
	CHECK(h->avail_buckets & BIT(bidx));

#ifdef CONFIG_SYS_HEAP_CANARIES
	struct z_heap_custom_header *custom_hdr_ptr =
	(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	uint64_t computed_header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, c, SIZE_AND_USED),
		cmem, h->magic_num_free);

	__ASSERT(custom_hdr_ptr->header_guard == computed_header_guard,
		 "corrupted memory");
#endif

	if (next_free_chunk(h, c) == c) {
		/* this is the last chunk */
		h->avail_buckets &= ~BIT(bidx);
		b->next = 0;
	} else {
		chunkid_t first = prev_free_chunk(h, c),
			  second = next_free_chunk(h, c);

		b->next = second;
		set_next_free_chunk(h, first, second);
		set_prev_free_chunk(h, second, first);
	}

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	h->free_bytes -= chunksz_to_bytes(h, chunk_size(h, c));
#endif
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

	if (b->next == 0U) {
		CHECK((h->avail_buckets & BIT(bidx)) == 0);

		/* Empty list, first item */
		h->avail_buckets |= BIT(bidx);
		b->next = c;
		set_prev_free_chunk(h, c, c);
		set_next_free_chunk(h, c, c);
	} else {
		CHECK(h->avail_buckets & BIT(bidx));

		/* Insert before (!) the "next" pointer */
		chunkid_t second = b->next;
		chunkid_t first = prev_free_chunk(h, second);

		set_prev_free_chunk(h, c, first);
		set_next_free_chunk(h, c, second);
		set_next_free_chunk(h, first, c);
		set_prev_free_chunk(h, second, c);
	}

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	h->free_bytes += chunksz_to_bytes(h, chunk_size(h, c));
#endif
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

	chunksz_t sz0 = chunk_size(h, lc);
	chunksz_t lsz = rc - lc;
	chunksz_t rsz = sz0 - lsz;

	set_chunk_size(h, lc, lsz);
	set_chunk_size(h, rc, rsz);
	set_left_chunk_size(h, rc, lsz);
	set_left_chunk_size(h, right_chunk(h, rc), rsz);
}

/* Does not modify free list */
static void merge_chunks(struct z_heap *h, chunkid_t lc, chunkid_t rc)
{
	chunksz_t newsz = chunk_size(h, lc) + chunk_size(h, rc);

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

#ifdef CONFIG_SYS_HEAP_CANARIES
	struct z_heap_custom_header *hdr_ptr =
		(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, c, SIZE_AND_USED),
		cmem, h->magic_num_free);
#endif
}

/*
 * Return the closest chunk ID corresponding to given memory pointer.
 * Here "closest" is only meaningful in the context of sys_heap_aligned_alloc()
 * where wanted alignment might not always correspond to a chunk header
 * boundary.
 */
static chunkid_t mem_to_chunkid(struct z_heap *h, void *p)
{
	uint8_t *mem_ptr = (uint8_t *)p;
	uint8_t *base = (uint8_t *)chunk_buf(h);

#ifdef CONFIG_SYS_HEAP_CANARIES
	chunkid_t id;

	/* Not on a chunk boundary: the chunk id is stored immediately before mem_ptr */
	if (((uintptr_t)mem_ptr % CHUNK_UNIT) != 0U) {
		/* Compute offset once, and validate the memcpy source range explicitly */
		size_t offset = (size_t)(mem_ptr - base);

		__ASSERT(offset >= sizeof(id), "underflow - invalid pointer");
		__ASSERT((const uint8_t *)mem_ptr <=
			 (const uint8_t *)chunk_buf(h) + ((size_t)h->end_chunk * CHUNK_UNIT),
			 "pointer beyond heap bounds");

		/* At this point, source start = base + (offset - sizeof(id)) >= base
		 * and source end = base + offset <= chunk_buf(h) + (h->end_chunk * CHUNK_UNIT)
		 */

		memcpy(&id, base + (offset - sizeof(id)), sizeof(id));

		__ASSERT(id <= h->end_chunk, "corrupted chunk id");

		return id;
	}

	/* On a chunk boundary: derive id from position after fixed header bytes */
	{
		size_t offset = (size_t)(mem_ptr - base);

		__ASSERT(offset >= (size_t)chunk_header_bytes(h) &&
			 (const uint8_t *)mem_ptr <= (const uint8_t *)chunk_buf(h)
			 + ((size_t)h->end_chunk * CHUNK_UNIT),
			 "pointer beyond heap bounds");

		id = (chunkid_t)((offset - (size_t)chunk_header_bytes(h)) / CHUNK_UNIT);

		__ASSERT(id <= h->end_chunk, "corrupted chunk id");

		return id;
	}
#else
	{
		size_t offset = (size_t)(mem_ptr - base);

		__ASSERT(offset >= (size_t)chunk_header_bytes(h) &&
			 mem_ptr <= (const uint8_t *)chunk_buf(h) +
			 ((size_t)h->end_chunk * CHUNK_UNIT),
			 "pointer beyond heap bounds");

		return (chunkid_t)((offset - (size_t)chunk_header_bytes(h)) / CHUNK_UNIT);
	}
#endif
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

#ifdef CONFIG_SYS_HEAP_CANARIES
	struct z_heap_custom_header *custom_hdr_ptr =
	(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	uint64_t computed_header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, c, SIZE_AND_USED),
		cmem, h->magic_num_used);

	__ASSERT(custom_hdr_ptr->header_guard == computed_header_guard,
		 "corrupted memory");
#endif

	set_chunk_used(h, c, false);
#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	h->allocated_bytes -= chunksz_to_bytes(h, chunk_size(h, c));
#endif

#ifdef CONFIG_SYS_HEAP_LISTENER
	heap_listener_notify_free(HEAP_ID_FROM_POINTER(heap), mem,
				  chunksz_to_bytes(h, chunk_size(h, c)));
#endif

	free_chunk(h, c);
}

size_t sys_heap_usable_size(struct sys_heap *heap, void *mem)
{
	struct z_heap *h = heap->heap;
	chunkid_t c = mem_to_chunkid(h, mem);
	size_t addr = (size_t)mem;
	size_t chunk_base = (size_t)&chunk_buf(h)[c];
	size_t chunk_sz = chunk_size(h, c) * CHUNK_UNIT;

	return chunk_sz - (addr - chunk_base);
}

static chunkid_t alloc_chunk(struct z_heap *h, chunksz_t sz)
{
	int bi = bucket_idx(h, sz);
	struct z_heap_bucket *b = &h->buckets[bi];

	CHECK(bi <= bucket_idx(h, h->end_chunk));

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
	if (b->next != 0U) {
		chunkid_t first = b->next;
		int i = CONFIG_SYS_HEAP_ALLOC_LOOPS;
		do {
			chunkid_t c = b->next;

#ifdef CONFIG_SYS_HEAP_CANARIES
	struct z_heap_custom_header *custom_hdr_ptr =
	(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	uint64_t computed_header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, c, SIZE_AND_USED),
		cmem, h->magic_num_free);

	__ASSERT(custom_hdr_ptr->header_guard == computed_header_guard,
		 "corrupted memory");
#endif
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
	uint32_t bmask = h->avail_buckets & ~BIT_MASK(bi + 1);

	if (bmask != 0U) {
		int minbucket = __builtin_ctz(bmask);
		chunkid_t c = h->buckets[minbucket].next;

		free_list_remove_bidx(h, c, minbucket);
		CHECK(chunk_size(h, c) >= sz);
		return c;
	}

	return 0;
}

void *sys_heap_alloc(struct sys_heap *heap, size_t bytes)
{
	struct z_heap *h = heap->heap;
	void *mem;
	chunksz_t actual_allocated_chunk_sz;

	if (bytes == 0U) {
		return NULL;
	}

	chunksz_t chunk_sz_orig = bytes_to_chunksz(h, bytes, 0);

#ifdef CONFIG_SYS_HEAP_CANARIES
	if (!big_heap(h)) {
		chunksz_t min_needed_sz = chunksz
			(Z_HEAP_CUSTOM_HEADER_BYTES +
			zephyr_base_header_bytes(h) +
			zephyr_free_list_pointers_bytes(h));
		if (chunk_sz_orig < min_needed_sz) {
			chunk_sz_orig = min_needed_sz;
		}
	}
#endif

	chunkid_t c = alloc_chunk(h, chunk_sz_orig);

	if (c == 0U) {
		return NULL;
	}

	chunksz_t allocated_chunk_sz = chunk_size(h, c);

#ifdef CONFIG_SYS_HEAP_CANARIES
	if ((allocated_chunk_sz - chunk_sz_orig) > 0 &&
	    (allocated_chunk_sz - chunk_sz_orig) >= min_chunk_size(h)) {
		chunkid_t new_free_chunk = c + chunk_sz_orig;

		split_chunks(h, c, new_free_chunk);

	struct z_heap_custom_header *hdr_ptr =
		(struct z_heap_custom_header *)chunk_buf(h)[new_free_chunk].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[new_free_chunk];

	hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, new_free_chunk, SIZE_AND_USED),
		cmem, h->magic_num_free);

		free_list_add(h, new_free_chunk);

		actual_allocated_chunk_sz = chunk_sz_orig;
	} else {
		actual_allocated_chunk_sz = allocated_chunk_sz;
	}
#else
	if (allocated_chunk_sz > chunk_sz_orig) {
		split_chunks(h, c, c + chunk_sz_orig);
		free_list_add(h, c + chunk_sz_orig);
		actual_allocated_chunk_sz = chunk_sz_orig;
	} else {
		actual_allocated_chunk_sz = allocated_chunk_sz;
	}
#endif

	set_chunk_used(h, c, true);

	mem = chunk_mem(h, c);

#ifdef CONFIG_SYS_HEAP_CANARIES
	struct z_heap_custom_header *hdr_ptr =
		(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, c, SIZE_AND_USED),
		cmem, h->magic_num_used);
#endif

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	increase_allocated_bytes(h, chunksz_to_bytes(h, chunk_size(h, c)));
#endif

#ifdef CONFIG_SYS_HEAP_LISTENER
	heap_listener_notify_alloc(HEAP_ID_FROM_POINTER(heap), mem,
				   chunksz_to_bytes(h, chunk_size(h, c)));
#endif

	IF_ENABLED(CONFIG_MSAN, (__msan_allocated_memory(mem, bytes)));
	return mem;
}

void *sys_heap_noalign_alloc(struct sys_heap *heap, size_t align, size_t bytes)
{
	ARG_UNUSED(align);

	return sys_heap_alloc(heap, bytes);
}

void *sys_heap_aligned_alloc(struct sys_heap *heap, size_t align, size_t bytes)
{
	struct z_heap *h = heap->heap;
	size_t gap, rew;
	#ifdef CONFIG_SYS_HEAP_CANARIES
	uint8_t *mem2 = NULL;
	#endif

	/*
	 * Split align and rewind values (if any).
	 * We allow for one bit of rewind in addition to the alignment
	 * value to efficiently accommodate z_alloc_helper().
	 * So if e.g. align = 0x28 (32 | 8) this means we align to a 32-byte
	 * boundary and then rewind 8 bytes.
	 */
	rew = align & -align;
	if (align != rew) {
		align -= rew;
		gap = min(rew, chunk_header_bytes(h));
	} else {
		if (align <= chunk_header_bytes(h)) {
			return sys_heap_alloc(heap, bytes);
		}
		rew = 0;
		gap = chunk_header_bytes(h);
	}
	__ASSERT((align & (align - 1)) == 0, "align must be a power of 2");

	if (bytes == 0) {
		return NULL;
	}

	/*
	 * Find a free block that is guaranteed to fit.
	 * We over-allocate to account for alignment and then free
	 * the extra allocations afterwards.
	 */
#ifdef CONFIG_SYS_HEAP_CANARIES
	size_t total_needed = bytes + align - gap + chunk_header_bytes(h);
	chunksz_t padded_sz = bytes_to_chunksz(h, total_needed, 0);

	if ((total_needed % CHUNK_UNIT) == 0U) {
		padded_sz += 1U;
	}
#else
	chunksz_t padded_sz = bytes_to_chunksz(h, bytes, align - gap);
#endif
	chunkid_t c0 = alloc_chunk(h, padded_sz);

	if (c0 == 0) {
		return NULL;
	}
	uint8_t *mem = chunk_mem(h, c0);

	/* Align allocated memory */
	mem = (uint8_t *) ROUND_UP(mem + rew, align) - rew;

#ifdef CONFIG_SYS_HEAP_CANARIES
	mem2 = mem;

	if (((uintptr_t)mem2 % CHUNK_UNIT) != 0) {
		size_t base_off = (size_t)(mem2 - (uint8_t *)chunk_buf(h));

		__ASSERT(base_off >= sizeof(chunkid_t), "corrupted memory pointer");
			 *((chunkid_t *)(mem2 - sizeof(chunkid_t))) = c0;
	}
#endif

	chunk_unit_t *end = (chunk_unit_t *) ROUND_UP(mem + bytes, CHUNK_UNIT);

	/* Get corresponding chunks */
	chunkid_t c = mem_to_chunkid(h, mem);
	chunkid_t c_end = end - chunk_buf(h);
	CHECK(c >= c0 && c  < c_end && c_end <= c0 + padded_sz);

	/* Split and free unused prefix */
	if (c > c0) {
#ifdef CONFIG_SYS_HEAP_CANARIES
		chunksz_t prefix_sz = c - c0;

		if (prefix_sz >= min_chunk_size(h)) {
#endif
			split_chunks(h, c0, c);
#ifdef CONFIG_SYS_HEAP_CANARIES
			struct z_heap_custom_header *hdr_ptr =
				(struct z_heap_custom_header *)chunk_buf(h)[c0].bytes;
			chunk_unit_t *buf = chunk_buf(h);
			void *cmem = &buf[c0];

			hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
				(chunk_field(h, c0, SIZE_AND_USED),
				cmem, h->magic_num_free);
#endif
			free_list_add(h, c0);

#ifdef CONFIG_SYS_HEAP_CANARIES
			if (((uintptr_t)mem2 % CHUNK_UNIT) != 0) {
				size_t base_off = (size_t)(mem2 - (uint8_t *)chunk_buf(h));

				__ASSERT(base_off >= sizeof(chunkid_t), "corrupted memory pointer");
					 *(chunkid_t *)(mem2 - sizeof(chunkid_t)) = c;
			}
		} else {
			c = c0;
		}
#endif
	}

	/* Split and free unused suffix */
	if (right_chunk(h, c) > c_end) {
#ifdef CONFIG_SYS_HEAP_CANARIES
		chunksz_t suffix_sz = right_chunk(h, c) - c_end;

		if (suffix_sz >= min_chunk_size(h)) {
#endif
			split_chunks(h, c, c_end);
#ifdef CONFIG_SYS_HEAP_CANARIES
			struct z_heap_custom_header *hdr_ptr =
				(struct z_heap_custom_header *)chunk_buf(h)[c_end].bytes;
			chunk_unit_t *buf = chunk_buf(h);
			void *cmem = &buf[c_end];

			hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
				(chunk_field(h, c_end, SIZE_AND_USED),
				cmem, h->magic_num_free);
#endif
			free_list_add(h, c_end);
#ifdef CONFIG_SYS_HEAP_CANARIES
		}
#endif
	}

	set_chunk_used(h, c, true);

#ifdef CONFIG_SYS_HEAP_CANARIES
	struct z_heap_custom_header *hdr_ptr =
		(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, c, SIZE_AND_USED),
		cmem, h->magic_num_used);

	__ASSERT(left_chunk(h, right_chunk(h, c)) == c,
		 "corrupted heap bounds (buffer overflow?) for memory at %p",
		 mem);

	__ASSERT(IS_ALIGNED((uintptr_t)(mem + rew), align),
		 "corrupted mem pointer: address not aligned (addr=%p, align=%zu)",
		 (void *)(mem + rew), (size_t)align);
#endif

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	increase_allocated_bytes(h, chunksz_to_bytes(h, chunk_size(h, c)));
#endif

#ifdef CONFIG_SYS_HEAP_LISTENER
	heap_listener_notify_alloc(HEAP_ID_FROM_POINTER(heap), mem,
				   chunksz_to_bytes(h, chunk_size(h, c)));
#endif

	IF_ENABLED(CONFIG_MSAN, (__msan_allocated_memory(mem, bytes)));
	return mem;
}

static bool inplace_realloc(struct sys_heap *heap, void *ptr, size_t bytes)
{
	struct z_heap *h = heap->heap;

	chunkid_t c = mem_to_chunkid(h, ptr);
	size_t align_gap = (uint8_t *)ptr - (uint8_t *)chunk_mem(h, c);

	chunksz_t chunks_need = bytes_to_chunksz(h, bytes, align_gap);

	if (chunk_size(h, c) == chunks_need) {
		/* We're good already */
		return true;
	}
#ifdef CONFIG_SYS_HEAP_CANARIES
	if (chunk_size(h, c) >= (chunks_need + min_chunk_size(h))) {
#else
	if (chunk_size(h, c) > chunks_need) {
#endif
		/* Shrink in place, split off and free unused suffix */
#ifdef CONFIG_SYS_HEAP_LISTENER
		size_t bytes_freed = chunksz_to_bytes(h, chunk_size(h, c));
#endif

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
		h->allocated_bytes -=
			(chunk_size(h, c) - chunks_need) * CHUNK_UNIT;
#endif

		split_chunks(h, c, c + chunks_need);
		set_chunk_used(h, c, true);
#ifdef CONFIG_SYS_HEAP_CANARIES
		struct z_heap_custom_header *hdr_ptr =
			(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;
		chunk_unit_t *buf = chunk_buf(h);
		void *cmem = &buf[c];

		hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
			(chunk_field(h, c, SIZE_AND_USED),
			cmem, h->magic_num_used);
#endif
		free_chunk(h, c + chunks_need);

#ifdef CONFIG_SYS_HEAP_LISTENER
		heap_listener_notify_alloc(HEAP_ID_FROM_POINTER(heap), ptr,
					   chunksz_to_bytes(h, chunk_size(h, c)));
		heap_listener_notify_free(HEAP_ID_FROM_POINTER(heap), ptr,
					  bytes_freed);
#endif

		return true;
	}

	chunkid_t rc = right_chunk(h, c);

	if (!chunk_used(h, rc) &&
	    (chunk_size(h, c) + chunk_size(h, rc) >= chunks_need)) {
		/* Expand: split the right chunk and append */
		chunksz_t split_size = chunks_need - chunk_size(h, c);
#ifdef CONFIG_SYS_HEAP_CANARIES
#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
		h->allocated_bytes -= chunk_size(h, c) * CHUNK_UNIT;
#endif
#else
#ifdef CONFIG_SYS_HEAP_LISTENER
		size_t bytes_freed = chunksz_to_bytes(h, chunk_size(h, c));
#endif
#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
		increase_allocated_bytes(h, split_size * CHUNK_UNIT);
#endif
#endif
		free_list_remove(h, rc);

#ifdef CONFIG_SYS_HEAP_CANARIES
		merge_chunks(h, c, rc);

		if (chunk_size(h, c) >= chunks_need + min_chunk_size(h)) {
			split_chunks(h, c, c + chunks_need);

			struct z_heap_custom_header *hdr_ptr =
				(struct z_heap_custom_header *)chunk_buf(h)[c + chunks_need].bytes;
			chunk_unit_t *buf = chunk_buf(h);
			void *cmem = &buf[c + chunks_need];

			hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
				(chunk_field(h, c + chunks_need, SIZE_AND_USED),
				cmem, h->magic_num_free);

			free_list_add(h, c + chunks_need);
		}
#else
		if (split_size < chunk_size(h, rc)) {
			split_chunks(h, rc, rc + split_size);
			free_list_add(h, rc + split_size);
		}

		merge_chunks(h, c, rc);
#endif
		set_chunk_used(h, c, true);
#ifdef CONFIG_SYS_HEAP_CANARIES

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
		h->allocated_bytes += chunk_size(h, c) * CHUNK_UNIT;
#endif

#endif

#ifdef CONFIG_SYS_HEAP_CANARIES
	struct z_heap_custom_header *hdr_ptr =
		(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, c, SIZE_AND_USED),
		cmem, h->magic_num_used);
#endif

#ifdef CONFIG_SYS_HEAP_LISTENER
		heap_listener_notify_alloc(HEAP_ID_FROM_POINTER(heap), ptr,
					  chunksz_to_bytes(h, chunk_size(h, c)));
		heap_listener_notify_free(HEAP_ID_FROM_POINTER(heap), ptr,
					  bytes_freed);
#endif

		return true;
	}

	return false;
}

void *sys_heap_realloc(struct sys_heap *heap, void *ptr, size_t bytes)
{
	/* special realloc semantics */
	if (ptr == NULL) {
		return sys_heap_alloc(heap, bytes);
	}
	if (bytes == 0) {
		sys_heap_free(heap, ptr);
		return NULL;
	}

	if (inplace_realloc(heap, ptr, bytes)) {
		return ptr;
	}

	/* In-place realloc was not possible: fallback to allocate and copy. */
	void *ptr2 = sys_heap_alloc(heap, bytes);

	if (ptr2 != NULL) {
		size_t prev_size = sys_heap_usable_size(heap, ptr);

		memcpy(ptr2, ptr, min(prev_size, bytes));
		sys_heap_free(heap, ptr);
	}
	return ptr2;
}

void *sys_heap_aligned_realloc(struct sys_heap *heap, void *ptr,
			       size_t align, size_t bytes)
{
	/* special realloc semantics */
	if (ptr == NULL) {
		return sys_heap_aligned_alloc(heap, align, bytes);
	}
	if (bytes == 0) {
		sys_heap_free(heap, ptr);
		return NULL;
	}

	__ASSERT((align & (align - 1)) == 0, "align must be a power of 2");

	if ((align == 0 || ((uintptr_t)ptr & (align - 1)) == 0) &&
	    inplace_realloc(heap, ptr, bytes)) {
		return ptr;
	}

	/*
	 * Either ptr is not sufficiently aligned for in-place realloc or
	 * in-place realloc was not possible: fallback to allocate and copy.
	 */
	void *ptr2 = sys_heap_aligned_alloc(heap, align, bytes);

	if (ptr2 != NULL) {
		size_t prev_size = sys_heap_usable_size(heap, ptr);

		memcpy(ptr2, ptr, min(prev_size, bytes));
		sys_heap_free(heap, ptr);
	}
	return ptr2;
}

void sys_heap_init(struct sys_heap *heap, void *mem, size_t bytes)
{
	IF_ENABLED(CONFIG_MSAN, (__sanitizer_dtor_callback(mem, bytes)));

	if (IS_ENABLED(CONFIG_SYS_HEAP_SMALL_ONLY)) {
		/* Must fit in a 15 bit count of HUNK_UNIT */
		__ASSERT(bytes / CHUNK_UNIT <= 0x7fffU, "heap size is too big");
	} else {
		/* Must fit in a 31 bit count of HUNK_UNIT */
		__ASSERT(bytes / CHUNK_UNIT <= 0x7fffffffU, "heap size is too big");
	}

	/* Reserve the end marker chunk's header */
	__ASSERT(bytes > heap_footer_bytes(bytes), "heap size is too small");
	bytes -= heap_footer_bytes(bytes);

	/* Round the start up, the end down */
	uintptr_t addr = ROUND_UP(mem, CHUNK_UNIT);
	uintptr_t end = ROUND_DOWN((uint8_t *)mem + bytes, CHUNK_UNIT);
	chunksz_t heap_sz = (end - addr) / CHUNK_UNIT;

	CHECK(end > addr);
	__ASSERT(heap_sz > chunksz(sizeof(struct z_heap)), "heap size is too small");

	struct z_heap *h = (struct z_heap *)addr;
	heap->heap = h;
	h->end_chunk = heap_sz;
	h->avail_buckets = 0;

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	h->free_bytes = 0;
	h->allocated_bytes = 0;
	h->max_allocated_bytes = 0;
#endif

#if CONFIG_SYS_HEAP_ARRAY_SIZE
	sys_heap_array_save(heap);
#endif

	int nb_buckets = bucket_idx(h, heap_sz) + 1;
	chunksz_t chunk0_size = chunksz(sizeof(struct z_heap) +
				     nb_buckets * sizeof(struct z_heap_bucket));

	__ASSERT(chunk0_size + min_chunk_size(h) <= heap_sz, "heap size is too small");

	for (int i = 0; i < nb_buckets; i++) {
		h->buckets[i].next = 0;
	}

	/* chunk containing our struct z_heap */
	set_chunk_size(h, 0, chunk0_size);
	set_left_chunk_size(h, 0, 0);
	set_chunk_used(h, 0, true);

	/* chunk containing the free heap */
	set_chunk_size(h, chunk0_size, heap_sz - chunk0_size);
	set_left_chunk_size(h, chunk0_size, chunk0_size);

	/* the end marker chunk */
	set_chunk_size(h, heap_sz, 0);
	set_left_chunk_size(h, heap_sz, heap_sz - chunk0_size);
	set_chunk_used(h, heap_sz, true);

	free_list_add(h, chunk0_size);

#ifdef CONFIG_SYS_HEAP_CANARIES
	/* Fallback to a predefined value in case sys_rand_get() fails */
	h->magic_num_used = Z_HEAP_MAGIC_NUM_USED;
	h->magic_num_free = Z_HEAP_MAGIC_NUM_FREE;

	sys_rand_get(&h->magic_num_used, sizeof(h->magic_num_used));
	sys_rand_get(&h->magic_num_free, sizeof(h->magic_num_free));

	struct z_heap_custom_header *hdr_ptr =
		(struct z_heap_custom_header *)chunk_buf(h)[chunk0_size].bytes;
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[chunk0_size];

	hdr_ptr->header_guard = COMPUTE_HEADER_GUARD
		(chunk_field(h, chunk0_size, SIZE_AND_USED),
		cmem, h->magic_num_free);
#endif
}

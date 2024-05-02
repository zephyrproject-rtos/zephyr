/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include "heap.h"

/* White-box sys_heap validation code.  Uses internal data structures.
 * Not expected to be useful in production apps.  This checks every
 * header field of every chunk and returns true if the totality of the
 * data structure is a valid heap.  It doesn't necessarily tell you
 * that it is the CORRECT heap given the history of alloc/free calls
 * that it can't inspect.  In a pathological case, you can imagine
 * something scribbling a copy of a previously-valid heap on top of a
 * running one and corrupting it. YMMV.
 */

#define VALIDATE(cond) do { if (!(cond)) { return false; } } while (0)

static bool in_bounds(struct z_heap *h, chunkid_t c)
{
	VALIDATE(c >= right_chunk(h, 0));
	VALIDATE(c < h->end_chunk);
	VALIDATE(chunk_size(h, c) < h->end_chunk);
	return true;
}

static bool valid_chunk(struct z_heap *h, chunkid_t c)
{
	VALIDATE(chunk_size(h, c) > 0);
	VALIDATE((c + chunk_size(h, c)) <= h->end_chunk);
	VALIDATE(in_bounds(h, c));
	VALIDATE(right_chunk(h, left_chunk(h, c)) == c);
	VALIDATE(left_chunk(h, right_chunk(h, c)) == c);
	if (chunk_used(h, c)) {
		VALIDATE(!solo_free_header(h, c));
	} else {
		VALIDATE(chunk_used(h, left_chunk(h, c)));
		VALIDATE(chunk_used(h, right_chunk(h, c)));
		if (!solo_free_header(h, c)) {
			VALIDATE(in_bounds(h, prev_free_chunk(h, c)));
			VALIDATE(in_bounds(h, next_free_chunk(h, c)));
		}
	}
	return true;
}

/* Validate multiple state dimensions for the bucket "next" pointer
 * and see that they match.  Probably should unify the design a
 * bit...
 */
static inline void check_nexts(struct z_heap *h, int bidx)
{
	struct z_heap_bucket *b = &h->buckets[bidx];

	bool emptybit = (h->avail_buckets & BIT(bidx)) == 0;
	bool emptylist = b->next == 0;
	bool empties_match = emptybit == emptylist;

	(void)empties_match;
	CHECK(empties_match);

	if (b->next != 0) {
		CHECK(valid_chunk(h, b->next));
	}
}

bool sys_heap_validate(struct sys_heap *heap)
{
	struct z_heap *h = heap->heap;
	chunkid_t c;

	/*
	 * Walk through the chunks linearly, verifying sizes and end pointer.
	 */
	for (c = right_chunk(h, 0); c < h->end_chunk; c = right_chunk(h, c)) {
		if (!valid_chunk(h, c)) {
			return false;
		}
	}
	if (c != h->end_chunk) {
		return false;  /* Should have exactly consumed the buffer */
	}

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	/*
	 * Validate sys_heap_runtime_stats_get API.
	 * Iterate all chunks in sys_heap to get total allocated bytes and
	 * free bytes, then compare with the results of
	 * sys_heap_runtime_stats_get function.
	 */
	size_t allocated_bytes, free_bytes;
	struct sys_memory_stats stat;

	get_alloc_info(h, &allocated_bytes, &free_bytes);
	sys_heap_runtime_stats_get(heap, &stat);
	if ((stat.allocated_bytes != allocated_bytes) ||
	    (stat.free_bytes != free_bytes)) {
		return false;
	}
#endif

	/* Check the free lists: entry count should match, empty bit
	 * should be correct, and all chunk entries should point into
	 * valid unused chunks.  Mark those chunks USED, temporarily.
	 */
	for (int b = 0; b <= bucket_idx(h, h->end_chunk); b++) {
		chunkid_t c0 = h->buckets[b].next;
		uint32_t n = 0;

		check_nexts(h, b);

		for (c = c0; c != 0 && (n == 0 || c != c0);
		     n++, c = next_free_chunk(h, c)) {
			if (!valid_chunk(h, c)) {
				return false;
			}
			set_chunk_used(h, c, true);
		}

		bool empty = (h->avail_buckets & BIT(b)) == 0;
		bool zero = n == 0;

		if (empty != zero) {
			return false;
		}

		if (empty && (h->buckets[b].next != 0)) {
			return false;
		}
	}

	/*
	 * Walk through the chunks linearly again, verifying that all chunks
	 * but solo headers are now USED (i.e. all free blocks were found
	 * during enumeration).  Mark all such blocks UNUSED and solo headers
	 * USED.
	 */
	chunkid_t prev_chunk = 0;

	for (c = right_chunk(h, 0); c < h->end_chunk; c = right_chunk(h, c)) {
		if (!chunk_used(h, c) && !solo_free_header(h, c)) {
			return false;
		}
		if (left_chunk(h, c) != prev_chunk) {
			return false;
		}
		prev_chunk = c;

		set_chunk_used(h, c, solo_free_header(h, c));
	}
	if (c != h->end_chunk) {
		return false;  /* Should have exactly consumed the buffer */
	}

	/* Go through the free lists again checking that the linear
	 * pass caught all the blocks and that they now show UNUSED.
	 * Mark them USED.
	 */
	for (int b = 0; b <= bucket_idx(h, h->end_chunk); b++) {
		chunkid_t c0 = h->buckets[b].next;
		int n = 0;

		if (c0 == 0) {
			continue;
		}

		for (c = c0; n == 0 || c != c0; n++, c = next_free_chunk(h, c)) {
			if (chunk_used(h, c)) {
				return false;
			}
			set_chunk_used(h, c, true);
		}
	}

	/* Now we are valid, but have managed to invert all the in-use
	 * fields.  One more linear pass to fix them up
	 */
	for (c = right_chunk(h, 0); c < h->end_chunk; c = right_chunk(h, c)) {
		set_chunk_used(h, c, !chunk_used(h, c));
	}
	return true;
}

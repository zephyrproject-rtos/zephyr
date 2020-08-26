/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/sys_heap.h>
#include <kernel.h>
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

static size_t max_chunkid(struct z_heap *h)
{
	return h->len - min_chunk_size(h);
}

#define VALIDATE(cond) do { if (!(cond)) { return false; } } while (0)

static bool in_bounds(struct z_heap *h, chunkid_t c)
{
	VALIDATE(c >= right_chunk(h, 0));
	VALIDATE(c <= max_chunkid(h));
	VALIDATE(chunk_size(h, c) < h->len);
	return true;
}

static bool valid_chunk(struct z_heap *h, chunkid_t c)
{
	VALIDATE(chunk_size(h, c) > 0);
	VALIDATE(c + chunk_size(h, c) <= h->len);
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

	bool emptybit = (h->avail_buckets & (1 << bidx)) == 0;
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
	for (c = right_chunk(h, 0); c <= max_chunkid(h); c = right_chunk(h, c)) {
		if (!valid_chunk(h, c)) {
			return false;
		}
	}
	if (c != h->len) {
		return false;  /* Should have exactly consumed the buffer */
	}

	/* Check the free lists: entry count should match, empty bit
	 * should be correct, and all chunk entries should point into
	 * valid unused chunks.  Mark those chunks USED, temporarily.
	 */
	for (int b = 0; b <= bucket_idx(h, h->len); b++) {
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

		bool empty = (h->avail_buckets & (1 << b)) == 0;
		bool zero = n == 0;

		if (empty != zero) {
			return false;
		}

		if (empty && h->buckets[b].next != 0) {
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
	for (c = right_chunk(h, 0); c <= max_chunkid(h); c = right_chunk(h, c)) {
		if (!chunk_used(h, c) && !solo_free_header(h, c)) {
			return false;
		}
		if (left_chunk(h, c) != prev_chunk) {
			return false;
		}
		prev_chunk = c;

		set_chunk_used(h, c, solo_free_header(h, c));
	}
	if (c != h->len) {
		return false;  /* Should have exactly consumed the buffer */
	}

	/* Go through the free lists again checking that the linear
	 * pass caught all the blocks and that they now show UNUSED.
	 * Mark them USED.
	 */
	for (int b = 0; b <= bucket_idx(h, h->len); b++) {
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
	for (c = right_chunk(h, 0); c <= max_chunkid(h); c = right_chunk(h, c)) {
		set_chunk_used(h, c, !chunk_used(h, c));
	}
	return true;
}

struct z_heap_stress_rec {
	void *(*alloc)(void *arg, size_t bytes);
	void (*free)(void *arg, void *p);
	void *arg;
	size_t total_bytes;
	struct z_heap_stress_block *blocks;
	size_t nblocks;
	size_t blocks_alloced;
	size_t bytes_alloced;
	uint32_t target_percent;
};

struct z_heap_stress_block {
	void *ptr;
	size_t sz;
};

/* Very simple LCRNG (from https://nuclear.llnl.gov/CNP/rng/rngman/node4.html)
 *
 * Here to guarantee cross-platform test repeatability.
 */
static uint32_t rand32(void)
{
	static uint64_t state = 123456789; /* seed */

	state = state * 2862933555777941757UL + 3037000493UL;

	return (uint32_t)(state >> 32);
}

static bool rand_alloc_choice(struct z_heap_stress_rec *sr)
{
	/* Edge cases: no blocks allocated, and no space for a new one */
	if (sr->blocks_alloced == 0) {
		return true;
	} else if (sr->blocks_alloced >= sr->nblocks) {
		return false;
	}

	/* The way this works is to scale the chance of choosing to
	 * allocate vs. free such that it's even odds when the heap is
	 * at the target percent, with linear tapering on the low
	 * slope (i.e. we choose to always allocate with an empty
	 * heap, allocate 50% of the time when the heap is exactly at
	 * the target, and always free when above the target).  In
	 * practice, the operations aren't quite symmetric (you can
	 * always free, but your allocation might fail), and the units
	 * aren't matched (we're doing math based on bytes allocated
	 * and ignoring the overhead) but this is close enough.  And
	 * yes, the math here is coarse (in units of percent), but
	 * that's good enough and fits well inside 32 bit quantities.
	 * (Note precision issue when heap size is above 40MB
	 * though!).
	 */
	__ASSERT(sr->total_bytes < 0xffffffffU / 100, "too big for u32!");
	uint32_t full_pct = (100 * sr->bytes_alloced) / sr->total_bytes;
	uint32_t target = sr->target_percent ? sr->target_percent : 1;
	uint32_t free_chance = 0xffffffffU;

	if (full_pct < sr->target_percent) {
		free_chance = full_pct * (0x80000000U / target);
	}

	return rand32() > free_chance;
}

/* Chooses a size of block to allocate, logarithmically favoring
 * smaller blocks (i.e. blocks twice as large are half as frequent
 */
static size_t rand_alloc_size(struct z_heap_stress_rec *sr)
{
	ARG_UNUSED(sr);

	/* Min scale of 4 means that the half of the requests in the
	 * smallest size have an average size of 8
	 */
	int scale = 4 + __builtin_clz(rand32());

	return rand32() & ((1 << scale) - 1);
}

/* Returns the index of a randomly chosen block to free */
static size_t rand_free_choice(struct z_heap_stress_rec *sr)
{
	return rand32() % sr->blocks_alloced;
}

/* General purpose heap stress test.  Takes function pointers to allow
 * for testing multiple heap APIs with the same rig.  The alloc and
 * free functions are passed back the argument as a context pointer.
 * The "log" function is for readable user output.  The total_bytes
 * argument should reflect the size of the heap being tested.  The
 * scratch array is used to store temporary state and should be sized
 * about half as large as the heap itself. Returns true on success.
 */
void sys_heap_stress(void *(*alloc)(void *arg, size_t bytes),
		     void (*free)(void *arg, void *p),
		     void *arg, size_t total_bytes,
		     uint32_t op_count,
		     void *scratch_mem, size_t scratch_bytes,
		     int target_percent,
		     struct z_heap_stress_result *result)
{
	struct z_heap_stress_rec sr = {
	       .alloc = alloc,
	       .free = free,
	       .arg = arg,
	       .total_bytes = total_bytes,
	       .blocks = scratch_mem,
	       .nblocks = scratch_bytes / sizeof(struct z_heap_stress_block),
	       .target_percent = target_percent,
	};

	*result = (struct z_heap_stress_result) {0};

	for (uint32_t i = 0; i < op_count; i++) {
		if (rand_alloc_choice(&sr)) {
			size_t sz = rand_alloc_size(&sr);
			void *p = sr.alloc(sr.arg, sz);

			result->total_allocs++;
			if (p != NULL) {
				result->successful_allocs++;
				sr.blocks[sr.blocks_alloced].ptr = p;
				sr.blocks[sr.blocks_alloced].sz = sz;
				sr.blocks_alloced++;
				sr.bytes_alloced += sz;
			}
		} else {
			int b = rand_free_choice(&sr);
			void *p = sr.blocks[b].ptr;
			size_t sz = sr.blocks[b].sz;

			result->total_frees++;
			sr.blocks[b] = sr.blocks[sr.blocks_alloced - 1];
			sr.blocks_alloced--;
			sr.bytes_alloced -= sz;
			sr.free(sr.arg, p);
		}
		result->accumulated_in_use_bytes += sr.bytes_alloced;
	}
}

/*
 * Dump heap structure content for debugging / analysis purpose
 */
void heap_dump(struct z_heap *h)
{
	int i, nb_buckets = bucket_idx(h, h->len) + 1;

	printk("Heap at %p contains %d units\n", chunk_buf(h), h->len);

	for (i = 0; i < nb_buckets; i++) {
		chunkid_t first = h->buckets[i].next;
		int count = 0;

		if (first) {
			chunkid_t curr = first;
			do {
				count++;
				curr = next_free_chunk(h, curr);
			} while (curr != first);
		}

		printk("bucket %d (min %d units): %d chunks\n", i,
		       (1 << i) - 1 + min_chunk_size(h), count);
	}

	for (chunkid_t c = 0; ; c = right_chunk(h, c)) {
		printk("chunk %3zd: %c %3zd] %3zd [%zd\n",
		       c, chunk_used(h, c) ? '*' : '-',
		      left_chunk(h, c), chunk_size(h, c), right_chunk(h, c));
		if (c == h->len) {
			break;
		}
	}
}

void sys_heap_dump(struct sys_heap *heap)
{
	heap_dump(heap->heap);
}

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include "heap.h"

struct z_heap_stress_rec {
	void *(*alloc_fn)(void *arg, size_t bytes);
	void (*free_fn)(void *arg, void *p);
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
	} else {

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

	return rand32() & BIT_MASK(scale);
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
void sys_heap_stress(void *(*alloc_fn)(void *arg, size_t bytes),
		     void (*free_fn)(void *arg, void *p),
		     void *arg, size_t total_bytes,
		     uint32_t op_count,
		     void *scratch_mem, size_t scratch_bytes,
		     int target_percent,
		     struct z_heap_stress_result *result)
{
	struct z_heap_stress_rec sr = {
	       .alloc_fn = alloc_fn,
	       .free_fn = free_fn,
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
			void *p = sr.alloc_fn(sr.arg, sz);

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
			sr.free_fn(sr.arg, p);
		}
		result->accumulated_in_use_bytes += sr.bytes_alloced;
	}
}

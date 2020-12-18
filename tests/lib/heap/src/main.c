/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <ztest.h>
#include <sys/sys_heap.h>

/* Guess at a value for heap size based on available memory on the
 * platform, with workarounds.
 */

#if defined(CONFIG_BOARD_MPS2_AN521)
/* mps2_an521 blows up if allowed to link into large area, even though
 * the link is successful and it claims the memory is there.  We get
 * hard faults on boot in qemu before entry to cstart() once MEMSZ is
 * allowed to get near 256kb.
 */
# define MEMSZ (192 * 1024)
#elif defined(CONFIG_ARCH_POSIX)
/* native_posix doesn't support CONFIG_SRAM_SIZE at all (because
 * it can link anything big enough to fit on the host), so just use a
 * reasonable value.
 */
# define MEMSZ (2 * 1024 * 1024)
#elif defined(CONFIG_SOC_ARC_EMSDP) || defined(CONFIG_SOC_EMSK)
/* Various ARC platforms set CONFIG_SRAM_SIZE to 16-128M, but have a
 * much lower limit of (32-64k) in their linker scripts.  Pick a
 * conservative fallback.
 */
# define MEMSZ (16 * 1024)
#else
/* Otherwise just trust CONFIG_SRAM_SIZE
 */
# define MEMSZ (1024 * (size_t) CONFIG_SRAM_SIZE)
#endif

#define BIG_HEAP_SZ MIN(256 * 1024, MEMSZ / 3)
#define SMALL_HEAP_SZ MIN(BIG_HEAP_SZ, 2048)
#define SCRATCH_SZ (sizeof(heapmem) / 2)

/* The test memory.  Make them pointer arrays for robust alignment
 * behavior
 */
void *heapmem[BIG_HEAP_SZ / sizeof(void *)];
void *scratchmem[SCRATCH_SZ / sizeof(void *)];

/* How many alloc/free operations are tested on each heap.  Two per
 * byte of heap sounds about right to get exhaustive coverage without
 * blowing too many cycles
 */
#define ITERATION_COUNT (2 * SMALL_HEAP_SZ)

/* Simple dumb hash function of the size and address */
static size_t fill_token(void *p, size_t sz)
{
	size_t pi = (size_t) p;

	return (pi * sz) ^ ((sz ^ 0xea6d) * ((pi << 11) | (pi >> 21)));
}

/* Puts markers at the start and end of a block to ensure that nothing
 * scribbled on it while it was allocated.  The first word is the
 * block size.  The second and last (if they fits) are a hashed "fill
 * token"
 */
static void fill_block(void *p, size_t sz)
{
	if (p == NULL) {
		return;
	}

	size_t tok = fill_token(p, sz);

	((size_t *)p)[0] = sz;

	if (sz >= 2 * sizeof(size_t)) {
		((size_t *)p)[1] = tok;
	}

	if (sz > 3*sizeof(size_t)) {
		((size_t *)p)[sz / sizeof(size_t) - 1] = tok;
	}
}

/* Checks markers just before freeing a block */
static void check_fill(void *p)
{
	size_t sz = ((size_t *)p)[0];
	size_t tok = fill_token(p, sz);

	zassert_true(sz > 0, "");

	if (sz >= 2 * sizeof(size_t)) {
		zassert_true(((size_t *)p)[1] == tok, "");
	}

	if (sz > 3 * sizeof(size_t)) {
		zassert_true(((size_t *)p)[sz / sizeof(size_t) - 1] == tok, "");
	}
}

void *testalloc(void *arg, size_t bytes)
{
	void *ret = sys_heap_alloc(arg, bytes);

	fill_block(ret, bytes);
	sys_heap_validate(arg);
	return ret;
}

void testfree(void *arg, void *p)
{
	check_fill(p);
	sys_heap_free(arg, p);
	sys_heap_validate(arg);
}

static void log_result(size_t sz, struct z_heap_stress_result *r)
{
	uint32_t tot = r->total_allocs + r->total_frees;
	uint32_t avg = (uint32_t)((r->accumulated_in_use_bytes + tot/2) / tot);
	uint32_t avg_pct = (uint32_t)((100ULL * avg + sz / 2) / sz);
	uint32_t succ_pct = ((100ULL * r->successful_allocs + r->total_allocs / 2)
			  / r->total_allocs);

	TC_PRINT("successful allocs: %d/%d (%d%%), frees: %d,"
		 "  avg usage: %d/%d (%d%%)\n",
		 r->successful_allocs, r->total_allocs, succ_pct,
		 r->total_frees, avg, (int) sz, avg_pct);
}

/* Do a heavy test over a small heap, with many iterations that need
 * to reuse memory repeatedly.  Target 50% fill, as that setting tends
 * to prevent runaway fragmentation and most allocations continue to
 * succeed in steady state.
 */
static void test_small_heap(void)
{
	struct sys_heap heap;
	struct z_heap_stress_result result;

	TC_PRINT("Testing small (%d byte) heap\n", (int) SMALL_HEAP_SZ);

	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);
	zassert_true(sys_heap_validate(&heap), "");
	sys_heap_stress(testalloc, testfree, &heap,
			SMALL_HEAP_SZ, ITERATION_COUNT,
			scratchmem, sizeof(scratchmem),
			50, &result);

	log_result(SMALL_HEAP_SZ, &result);
}

/* Very similar, but tests a fragmentation runaway scenario where we
 * target 100% fill and end up breaking memory up into maximally
 * fragmented blocks (i.e. small allocations always grab and split the
 * bigger chunks).  Obviously success rates in alloc will be very low,
 * but consistency should still be maintained.  Paradoxically, fill
 * level is not much better than the 50% target due to all the
 * fragmentation overhead (also the way we do accounting: we are
 * counting bytes requested, so if you ask for a 3 byte block and
 * receive a 8 byte minimal chunk, we still count that as 5 bytes of
 * waste).
 */
static void test_fragmentation(void)
{
	struct sys_heap heap;
	struct z_heap_stress_result result;

	TC_PRINT("Testing maximally fragmented (%d byte) heap\n",
		 (int) SMALL_HEAP_SZ);

	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);
	zassert_true(sys_heap_validate(&heap), "");
	sys_heap_stress(testalloc, testfree, &heap,
			SMALL_HEAP_SZ, ITERATION_COUNT,
			scratchmem, sizeof(scratchmem),
			100, &result);

	log_result(SMALL_HEAP_SZ, &result);
}

/* The heap block format changes for heaps with more than 2^15 chunks,
 * so test that case too.  This can be too large to iterate over
 * exhaustively with good performance, so the relative operation count
 * and fragmentation is going to be lower.
 */
static void test_big_heap(void)
{
	struct sys_heap heap;
	struct z_heap_stress_result result;

	TC_PRINT("Testing big (%d byte) heap\n", (int) BIG_HEAP_SZ);

	sys_heap_init(&heap, heapmem, BIG_HEAP_SZ);
	zassert_true(sys_heap_validate(&heap), "");
	sys_heap_stress(testalloc, testfree, &heap,
			BIG_HEAP_SZ, ITERATION_COUNT,
			scratchmem, sizeof(scratchmem),
			100, &result);

	log_result(BIG_HEAP_SZ, &result);
}

/* Simple clobber detection */
void realloc_fill_block(uint8_t *p, size_t sz)
{
	uint8_t val = (uint8_t)((uintptr_t)p >> 3);

	for (int i = 0; i < sz; i++) {
		p[i] = (uint8_t)(val + i);
	}
}

bool realloc_check_block(uint8_t *data, uint8_t *orig, size_t sz)
{
	uint8_t val = (uint8_t)((uintptr_t)orig >> 3);

	for (int i = 0; i < sz; i++) {
		if (data[i] != (uint8_t)(val + i)) {
			return false;
		}
	}
	return true;
}

static void test_realloc(void)
{
	struct sys_heap heap;
	void *p1, *p2, *p3;

	/* Note whitebox assumption: allocation goes from low address
	 * to high in an empty heap.
	 */

	sys_heap_init(&heap, heapmem, SMALL_HEAP_SZ);

	/* Allocate from an empty heap, then expand, validate that it
	 * happens in place.
	 */
	p1 = sys_heap_alloc(&heap, 64);
	realloc_fill_block(p1, 64);
	p2 = sys_heap_realloc(&heap, p1, 128);

	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(p1 == p2,
		     "Realloc should have expanded in place %p -> %p",
		     p1, p2);
	zassert_true(realloc_check_block(p2, p1, 64), "data changed");

	/* Allocate two blocks, then expand the first, validate that
	 * it moves.
	 */
	p1 = sys_heap_alloc(&heap, 64);
	realloc_fill_block(p1, 64);
	p2 = sys_heap_alloc(&heap, 64);
	realloc_fill_block(p2, 64);
	p3 = sys_heap_realloc(&heap, p1, 128);

	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(p1 != p2,
		     "Realloc should must have moved %p", p1);
	zassert_true(realloc_check_block(p2, p2, 64), "data changed");
	zassert_true(realloc_check_block(p3, p1, 64), "data changed");

	/* Allocate, then shrink.  Validate that it does not move. */
	p1 = sys_heap_alloc(&heap, 128);
	realloc_fill_block(p1, 128);
	p2 = sys_heap_realloc(&heap, p1, 64);

	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(p1 == p2,
		     "Realloc should have shrunk in place %p -> %p",
		     p1, p2);
	zassert_true(realloc_check_block(p2, p1, 64), "data changed");

	/* Allocate two blocks, then expand the first within a chunk.
	 * validate that it doesn't move. We assume CHUNK_UNIT == 8.
	 */
	p1 = sys_heap_alloc(&heap, 61);
	realloc_fill_block(p1, 61);
	p2 = sys_heap_alloc(&heap, 80);
	realloc_fill_block(p2, 80);
	p3 = sys_heap_realloc(&heap, p1, 64);

	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(p1 == p3,
		     "Realloc should have expanded in place %p -> %p",
		     p1, p3);
	zassert_true(realloc_check_block(p3, p1, 61), "data changed");
}

void test_main(void)
{
	ztest_test_suite(lib_heap_test,
			 ztest_unit_test(test_realloc),
			 ztest_unit_test(test_small_heap),
			 ztest_unit_test(test_fragmentation),
			 ztest_unit_test(test_big_heap)
			 );

	ztest_run_test_suite(lib_heap_test);
}

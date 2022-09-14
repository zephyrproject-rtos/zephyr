/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/heap_listener.h>
#include <inttypes.h>

/* Guess at a value for heap size based on available memory on the
 * platform, with workarounds.
 */

#if defined(CONFIG_SOC_MPS2_AN521) && defined(CONFIG_QEMU_TARGET)
/* mps2/an521 blows up if allowed to link into large area, even though
 * the link is successful and it claims the memory is there.  We get
 * hard faults on boot in qemu before entry to cstart() once MEMSZ is
 * allowed to get near 256kb.
 */
# define MEMSZ (192 * 1024)
#elif defined(CONFIG_ARCH_POSIX)
/* POSIX arch based targets don't support CONFIG_SRAM_SIZE at all (because
 * they can link anything big enough to fit on the host), so just use a
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

/* With enabling SYS_HEAP_RUNTIME_STATS, the size of struct z_heap
 * will increase 16 bytes on 64 bit CPU.
 */
#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
#define SOLO_FREE_HEADER_HEAP_SZ (80)
#else
#define SOLO_FREE_HEADER_HEAP_SZ (64)
#endif

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

	if (ret != NULL) {
		/* White box: the heap internals will allocate memory
		 * in 8 chunk units, no more than needed, but with a
		 * header prepended that is 4 or 8 bytes.  Use this to
		 * validate the block_size predicate.
		 */
		size_t blksz = sys_heap_usable_size(arg, ret);
		size_t addr = (size_t) ret;
		size_t chunk = ROUND_DOWN(addr - 1, 8);
		size_t hdr = addr - chunk;
		size_t expect = ROUND_UP(bytes + hdr, 8) - hdr;

		zassert_equal(blksz, expect,
			      "wrong size block returned bytes = %ld ret = %ld",
			      bytes, blksz);
	}

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
ZTEST(lib_heap, test_small_heap)
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
ZTEST(lib_heap, test_fragmentation)
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
ZTEST(lib_heap, test_big_heap)
{
	struct sys_heap heap;
	struct z_heap_stress_result result;

	if (IS_ENABLED(CONFIG_SYS_HEAP_SMALL_ONLY)) {
		TC_PRINT("big heap support is disabled\n");
		ztest_test_skip();
	}

	TC_PRINT("Testing big (%d byte) heap\n", (int) BIG_HEAP_SZ);

	sys_heap_init(&heap, heapmem, BIG_HEAP_SZ);
	zassert_true(sys_heap_validate(&heap), "");
	sys_heap_stress(testalloc, testfree, &heap,
			BIG_HEAP_SZ, ITERATION_COUNT,
			scratchmem, sizeof(scratchmem),
			100, &result);

	log_result(BIG_HEAP_SZ, &result);
}

/* Test a heap with a solo free header.  A solo free header can exist
 * only on a heap with 64 bit CPU (or chunk_header_bytes() == 8).
 * With 64 bytes heap and 1 byte allocation on a big heap, we get:
 *
 *   0   1   2   3   4   5   6   7
 * | h | h | b | b | c | 1 | s | f |
 *
 * where
 * - h: chunk0 header
 * - b: buckets in chunk0
 * - c: chunk header for the first allocation
 * - 1: chunk mem
 * - s: solo free header
 * - f: end marker / footer
 */
ZTEST(lib_heap, test_solo_free_header)
{
	struct sys_heap heap;

	TC_PRINT("Testing solo free header in a heap\n");

	sys_heap_init(&heap, heapmem, SOLO_FREE_HEADER_HEAP_SZ);
	if (sizeof(void *) > 4U) {
		sys_heap_alloc(&heap, 1);
		zassert_true(sys_heap_validate(&heap), "");
	} else {
		ztest_test_skip();
	}
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

ZTEST(lib_heap, test_realloc)
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
		     "Realloc should have moved %p", p1);
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

	/* Corner case with sys_heap_aligned_realloc() on 32-bit targets
	 * where actual memory doesn't match with given pointer
	 * (align_gap != 0).
	 */
	p1 = sys_heap_aligned_alloc(&heap, 8, 32);
	realloc_fill_block(p1, 32);
	p2 = sys_heap_alloc(&heap, 32);
	realloc_fill_block(p2, 32);
	p3 = sys_heap_aligned_realloc(&heap, p1, 8, 36);

	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(realloc_check_block(p3, p1, 32), "data changed");
	zassert_true(realloc_check_block(p2, p2, 32), "data changed");
	realloc_fill_block(p3, 36);
	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(p1 != p3,
		     "Realloc should have moved %p", p1);

	/* Test realloc with increasing alignment */
	p1 = sys_heap_aligned_alloc(&heap, 32, 32);
	p2 = sys_heap_aligned_alloc(&heap, 8, 32);
	p3 = sys_heap_aligned_realloc(&heap, p2, 8, 16);
	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(p2 == p3,
		     "Realloc should have expanded in place %p -> %p",
		     p2, p3);
	p3 = sys_heap_aligned_alloc(&heap, 32, 8);
	zassert_true(sys_heap_validate(&heap), "invalid heap");
	zassert_true(p2 != p3,
		     "Realloc should have moved %p", p2);
}

#ifdef CONFIG_SYS_HEAP_LISTENER
static struct sys_heap listener_heap;
static uintptr_t listener_heap_id;
static void *listener_mem;

static void heap_alloc_cb(uintptr_t heap_id, void *mem, size_t bytes)
{
	listener_heap_id = heap_id;
	listener_mem = mem;

	TC_PRINT("Heap 0x%" PRIxPTR ", alloc %p, size %u\n",
		 heap_id, mem, (uint32_t)bytes);
}

static void heap_free_cb(uintptr_t heap_id, void *mem, size_t bytes)
{
	listener_heap_id = heap_id;
	listener_mem = mem;

	TC_PRINT("Heap 0x%" PRIxPTR ", free %p, size %u\n",
		 heap_id, mem, (uint32_t)bytes);
}
#endif /* CONFIG_SYS_HEAP_LISTENER */

ZTEST(lib_heap, test_heap_listeners)
{
#ifdef CONFIG_SYS_HEAP_LISTENER
	void *mem;

	HEAP_LISTENER_ALLOC_DEFINE(heap_event_alloc,
				   HEAP_ID_FROM_POINTER(&listener_heap),
				   heap_alloc_cb);

	HEAP_LISTENER_FREE_DEFINE(heap_event_free,
				  HEAP_ID_FROM_POINTER(&listener_heap),
				  heap_free_cb);

	sys_heap_init(&listener_heap, heapmem, SMALL_HEAP_SZ);

	/* Register listeners */
	heap_listener_register(&heap_event_alloc);
	heap_listener_register(&heap_event_free);

	/*
	 * Note that sys_heap may allocate a bigger size than requested
	 * due to how sys_heap works. So checking whether the allocated
	 * size equals to the requested size does not work.
	 */

	/* Alloc/free operations without explicit alignment */
	mem = sys_heap_alloc(&listener_heap, 32U);

	zassert_equal(listener_heap_id,
		      HEAP_ID_FROM_POINTER(&listener_heap),
		      "Heap ID mismatched: 0x%lx != %p", listener_heap_id,
		      &listener_heap);
	zassert_equal(listener_mem, mem,
		      "Heap allocated pointer mismatched: %p != %p",
		      listener_mem, mem);

	sys_heap_free(&listener_heap, mem);
	zassert_equal(listener_heap_id,
		      HEAP_ID_FROM_POINTER(&listener_heap),
		      "Heap ID mismatched: 0x%lx != %p", listener_heap_id,
		      &listener_heap);
	zassert_equal(listener_mem, mem,
		      "Heap allocated pointer mismatched: %p != %p",
		      listener_mem, mem);

	/* Alloc/free operations with explicit alignment */
	mem = sys_heap_aligned_alloc(&listener_heap, 128U, 32U);

	zassert_equal(listener_heap_id,
		      HEAP_ID_FROM_POINTER(&listener_heap),
		      "Heap ID mismatched: 0x%lx != %p", listener_heap_id,
		      &listener_heap);
	zassert_equal(listener_mem, mem,
		      "Heap allocated pointer mismatched: %p != %p",
		      listener_mem, mem);

	sys_heap_free(&listener_heap, mem);
	zassert_equal(listener_heap_id,
		      HEAP_ID_FROM_POINTER(&listener_heap),
		      "Heap ID mismatched: 0x%lx != %p", listener_heap_id,
		      &listener_heap);
	zassert_equal(listener_mem, mem,
		      "Heap allocated pointer mismatched: %p != %p",
		      listener_mem, mem);

	/* Clean up */
	heap_listener_unregister(&heap_event_alloc);
	heap_listener_unregister(&heap_event_free);

#else /* CONFIG_SYS_HEAP_LISTENER */
	ztest_test_skip();
#endif /* CONFIG_SYS_HEAP_LISTENER */
}

ZTEST_SUITE(lib_heap, NULL, NULL, NULL, NULL, NULL);

/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/heap_kasan.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

static jmp_buf g_violation_jmp;
static volatile bool g_expect_violation;

/*
 * Override the weak heap_kasan_report(): longjmp before the write executes so
 * the out-of-bounds store never corrupts heap metadata.
 */
void heap_kasan_report(uintptr_t addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	if (g_expect_violation) {
		g_expect_violation = false;
		longjmp(g_violation_jmp, 1);
	}
	TC_PRINT("UNEXPECTED ASAN fault: addr=0x%lx size=%zu\n", (unsigned long)addr, size);
	ztest_test_fail();
}

#define ASSERT_VIOLATION(expr)					\
	do {										\
		g_expect_violation = true;				\
		if (setjmp(g_violation_jmp) == 0) {		\
			(void)(expr);				        \
			g_expect_violation = false;		    \
			ztest_test_fail();			        \
		}					                    \
	} while (0)

__attribute__((noinline)) void do_write(volatile uint8_t *p, size_t off, uint8_t val)
{
	p[off] = val;
}

__attribute__((noinline)) void do_write8(volatile uint8_t *p, size_t off, uint64_t val)
{
	typedef uint64_t __aligned(1) u64_unaligned_t;

	*(volatile u64_unaligned_t *)(p + off) = val;
}

static const uint8_t g_src_bytes[64];

__attribute__((noinline)) static void do_memset(void *p, int c, size_t n)
{
	memset(p, c, n);
}

__attribute__((noinline)) static void do_memcpy(void *dst, const void *src, size_t n)
{
	memcpy(dst, src, n);
}

__attribute__((noinline)) static void do_memmove(void *dst, const void *src, size_t n)
{
	memmove(dst, src, n);
}

static const char g_str_overflow[] = "ABCDEFGHIJKLMNOPQ"; /* 17 chars */
static const char g_str_exact[] = "ABCDEFGHIJKLMNO";      /* 15 chars */
static const char g_str_half[] = "abcdefgh";              /*  8 chars */
static const char g_str_append_overflow[] = "IJKLMNOPQ"; /*  9 chars */
static const char g_str_append_exact[] = "IJKLMNO";       /*  7 chars */

__attribute__((noinline)) static void do_strcpy(char *dst, const char *src)
{
	strcpy(dst, src);
}

__attribute__((noinline)) static void do_strncpy(char *dst, const char *src, size_t n)
{
	strncpy(dst, src, n);
}

__attribute__((noinline)) static void do_strcat(char *dst, const char *src)
{
	strcat(dst, src);
}

__attribute__((noinline)) static void do_strncat(char *dst, const char *src, size_t n)
{
	strncat(dst, src, n);
}

__attribute__((noinline)) static void do_snprintf(char *dst, size_t n, const char *src)
{
	snprintf(dst, n, "%s", src);
}

#if defined(CONFIG_SYS_HEAP_KASAN_EXTENSIONS)

__attribute__((noinline)) static void do_strlcpy(char *dst, const char *src, size_t siz)
{
	strlcpy(dst, src, siz);
}

__attribute__((noinline)) static void do_strlcat(char *dst, const char *src, size_t siz)
{
	strlcat(dst, src, siz);
}

__attribute__((noinline)) static char *do_stpcpy(char *dst, const char *src)
{
	return stpcpy(dst, src);
}

__attribute__((noinline)) static char *do_stpncpy(char *dst, const char *src, size_t n)
{
	return stpncpy(dst, src, n);
}

__attribute__((noinline)) static void *do_memccpy(void *dst, const void *src, int c, size_t n)
{
	return memccpy(dst, src, c, n);
}

__attribute__((noinline)) static void *do_mempcpy(void *dst, const void *src, size_t n)
{
	return mempcpy(dst, src, n);
}

__attribute__((noinline)) static char *do_fgets(char *buf, int size, FILE *stream)
{
	return fgets(buf, size, stream);
}

#endif /* CONFIG_SYS_HEAP_KASAN_EXTENSIONS */

struct heap_ops {
	void *(*alloc)(size_t bytes);
	void (*free)(void *ptr);
	void *(*realloc)(void *ptr, size_t bytes);
	int underflow_off;
	size_t alloc_overhead; /* bytes prepended before user pointer (e.g. k_heap*) */
};

#if defined(CONFIG_SYS_HEAP_KASAN_MALLOC)
struct asan_malloc_fixture {
	struct heap_ops ops;
};

static void *w_malloc(size_t b)
{
	return malloc(b);
}
static void w_free(void *p)
{
	free(p);
}
static void *w_realloc(void *p, size_t b)
{
	return realloc(p, b);
}

static void *malloc_setup(void)
{
	static struct asan_malloc_fixture f = {
		.ops = {.alloc = w_malloc,
			.free = w_free,
			.realloc = w_realloc,
			.underflow_off = -1,
			.alloc_overhead = 0},
	};
	return &f;
}

/* Skip when small heap is in use and malloc's alignment < GRANULE: the simple
 * alloc path returns pointers at offset chunk_header_bytes(4) within each
 * 8-byte granule, making overflow detection unreliable.
 */
static void malloc_before(void *state)
{
	ARG_UNUSED(state);
	bool small_heap = !(IS_ENABLED(CONFIG_SYS_HEAP_BIG_ONLY) || sizeof(void *) > 4U)
			  && (CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE / 8U <= 0x7fffU);

	if (small_heap && __alignof__(z_max_align_t) < SYS_HEAP_KASAN_GRANULE) {
		ztest_test_skip();
	}
}
#endif

#if defined(CONFIG_SYS_HEAP_KASAN_SYSTEM)
struct asan_sys_heap_fixture {
	struct heap_ops ops;
};

static void *w_k_malloc(size_t b)
{
	return k_malloc(b);
}
static void w_k_free(void *p)
{
	k_free(p);
}
static void *w_k_realloc(void *p, size_t b)
{
	return k_realloc(p, b);
}

static void *sys_heap_setup(void)
{
	static struct asan_sys_heap_fixture f = {
		.ops = {.alloc = w_k_malloc,
			.free = w_k_free,
			.realloc = w_k_realloc,
			.underflow_off = -(int)(sizeof(struct k_heap *) + 1),
			.alloc_overhead = sizeof(struct k_heap *)},
	};
	return &f;
}

/* On 32-bit + GRANULE>=8, k_malloc's 4-byte prepended pointer misaligns the
 * user pointer within a granule, making overflow detection unreliable.
 */
static void sys_heap_before(void *state)
{
	ARG_UNUSED(state);
	if (sizeof(void *) < SYS_HEAP_KASAN_GRANULE) {
		ztest_test_skip();
	}
}
#endif

static void run_normal(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	for (int i = 0; i < 16; i++) {
		do_write(p, i, (uint8_t)i);
	}
	ops->free(p);
}

static void run_overflow(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_write(p, 16, 0xFF));
	ops->free(p);
}

static void run_overflow_no_false_positive(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_write(p, 15, 0xFE);
	ops->free(p);
}

/* 8-byte overflow: __asan_store8 detects a write past a 16-byte block. */
static void run_overflow_8byte(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_write8(p, 16, 0xDEADC0DEULL));
	ops->free(p);
}

static void run_overflow_8byte_no_false_positive(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_write8(p, 8, 0xCAFEBABEULL);
	ops->free(p);
}

static void run_overflow_8byte_at_slot7(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(32);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_write8(p, 28, 0xDEADBEEFULL));
	ops->free(p);
}

/* No-FP counterpart: alloc(64) keeps bytes 28-35 within the allocation. */
static void run_overflow_8byte_at_slot7_no_false_positive(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(64);

	zassert_not_null(p, "alloc failed");
	do_write8(p, 28, 0xCAFEBABEULL);
	ops->free(p);
}

static void run_overflow_8byte_at_slot31(struct heap_ops *ops)
{
	if (SYS_HEAP_KASAN_GRANULE >= 8U) {
		ztest_test_skip();
		return;
	}

	uint8_t *p = ops->alloc(32U * SYS_HEAP_KASAN_GRANULE); /* 32 slots */

	zassert_not_null(p, "alloc(128) failed");
	ASSERT_VIOLATION(do_write8(p, 31U * SYS_HEAP_KASAN_GRANULE, 0xDEADBEEFULL));
	ops->free(p);
}

/* No-FP: allocate enough slots to cover all bytes of the 8-byte write. */
static void run_overflow_8byte_at_slot31_no_false_positive(struct heap_ops *ops)
{
	size_t n_slots = 31U + DIV_ROUND_UP(8U, SYS_HEAP_KASAN_GRANULE) + 1U;
	size_t alloc_bytes = n_slots * SYS_HEAP_KASAN_GRANULE;
	uint8_t *p = ops->alloc(alloc_bytes);

	zassert_not_null(p, "alloc failed");
	do_write8(p, 31U * SYS_HEAP_KASAN_GRANULE, 0xCAFEBABEULL);
	ops->free(p);
}

static void run_overflow_misaligned_granule_span(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(8);

	zassert_not_null(p, "alloc(8) failed");
	ASSERT_VIOLATION(do_write8(p, 3, 0xDEADC0DEULL));
	ops->free(p);
}

/* No-FP: alloc(16) opens granules 0-3, so p[3..10] is fully accessible. */
static void run_overflow_misaligned_granule_span_no_false_positive(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc(16) failed");
	do_write8(p, 3, 0xCAFEBABEULL);
	ops->free(p);
}

static void run_overflow_memset_full_bundle(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(32U * SYS_HEAP_KASAN_GRANULE);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_memset(p, 0, 32U * SYS_HEAP_KASAN_GRANULE + 1));
	ops->free(p);
}

static void run_overflow_memset_two_bundles(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(64U * SYS_HEAP_KASAN_GRANULE);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_memset(p, 0, 64U * SYS_HEAP_KASAN_GRANULE + 1));
	ops->free(p);
}

static void run_underflow(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	volatile uint8_t *vp = p;

	ASSERT_VIOLATION(do_write(vp + ops->underflow_off, 0, 0xDD));
	ops->free(p);
}

static void run_use_after_free(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ops->free(p);
	ASSERT_VIOLATION(do_write(p, 0, 0xBB));
}

static void run_use_after_free_memset(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ops->free(p);
	ASSERT_VIOLATION(do_memset(p, 0, 1));
}

static void run_use_after_free_memcpy(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ops->free(p);
	ASSERT_VIOLATION(do_memcpy(p, g_src_bytes, 1));
}

/* UAF via interior pointer: free() must poison the entire chunk, not just byte 0. */
static void run_use_after_free_interior_ptr(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_write(p, 12, 0xAA); /* must not trigger: p[12] is live */
	ops->free(p);
	ASSERT_VIOLATION(do_write(p, 12, 0xDE));
}

static void run_realloc_shrink_to_one_granule_boundary(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(64);

	zassert_not_null(p, "alloc(64) failed");

	uint8_t *q = ops->realloc(p, 1);

	zassert_not_null(q, "realloc(p, 1) failed");
	do_write(q, SYS_HEAP_KASAN_GRANULE - 1, 0xAA); /* must not trigger */
	ASSERT_VIOLATION(do_write(q, SYS_HEAP_KASAN_GRANULE, 0xBB));
	ops->free(q);
}

static void run_realloc_shrink_suffix(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(64);

	zassert_not_null(p, "alloc failed");
	uint8_t *q = ops->realloc(p, 16);

	zassert_not_null(q, "realloc failed");
	ASSERT_VIOLATION(do_write(q, 16, 0xCC));
	ops->free(q);
}

static void run_realloc_shrink_no_false_positive(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(64);

	zassert_not_null(p, "alloc failed");
	uint8_t *q = ops->realloc(p, 16);

	zassert_not_null(q, "realloc failed");
	for (int i = 0; i < 16; i++) {
		do_write(q, i, (uint8_t)i);
	}
	ops->free(q);
}

static void run_realloc_expand(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	uint8_t *q = ops->realloc(p, 64);

	zassert_not_null(q, "realloc expand failed");
	ASSERT_VIOLATION(do_write(q, 64, 0xEE));
	ops->free(q);
}

static void run_realloc_expand_no_false_positive(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	uint8_t *q = ops->realloc(p, 64);

	zassert_not_null(q, "realloc expand failed");
	for (int i = 0; i < 64; i++) {
		do_write(q, i, (uint8_t)i);
	}
	ops->free(q);
}

static void run_realloc_free_use_after(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	void *q = ops->realloc(p, 0);

	zassert_is_null(q, "realloc(p,0) should return NULL");
	ASSERT_VIOLATION(do_write(p, 0, 0xBB));
}

static void run_use_after_realloc_moved(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(8);
	uint8_t *blocker = ops->alloc(8);

	zassert_not_null(p, "alloc p failed");
	zassert_not_null(blocker, "alloc blocker failed");

	uint8_t *q = ops->realloc(p, 256);

	zassert_not_null(q, "realloc(p, 256) failed");

	if (q == p) {
		ops->free(q);
		ops->free(blocker);
		ztest_test_skip();
		return;
	}

	ASSERT_VIOLATION(do_write(p, 0, 0xDE));
	for (int i = 0; i < 256; i++) {
		do_write(q, i, (uint8_t)i);
	}
	ops->free(q);
	ops->free(blocker);
}

static void run_memset_overflow(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_memset(p, 0xAA, 17));
	ops->free(p);
}

static void run_memset_exact(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memset(p, 0, 16);
	ops->free(p);
}

static void run_memcpy_overflow(struct heap_ops *ops)
{
	uint8_t *dst = ops->alloc(16);

	zassert_not_null(dst, "alloc failed");
	ASSERT_VIOLATION(do_memcpy(dst, g_src_bytes, 17));
	ops->free(dst);
}

static void run_memcpy_exact(struct heap_ops *ops)
{
	uint8_t *dst = ops->alloc(16);

	zassert_not_null(dst, "alloc failed");
	do_memcpy(dst, g_src_bytes, 16);
	ops->free(dst);
}

static void run_memmove_overflow(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_memmove(p, g_src_bytes, 17));
	ops->free(p);
}

static void run_memmove_exact(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memmove(p, g_src_bytes, 16);
	ops->free(p);
}

static void run_memset_mid_overflow(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_memset(p + 8, 0xA5, 9));
	ops->free(p);
}

static void run_memset_mid_exact(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memset(p + 8, 0xC3, 8);
	ops->free(p);
}

/* alloc(1): bytes 0..GRANULE-1 accessible (granule rounding); byte GRANULE poisoned. */
static void run_granule_single_slot(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(1);

	zassert_not_null(p, "alloc(1) failed");
	do_write(p, SYS_HEAP_KASAN_GRANULE - 1, 0xA1); /* must not trigger */
	ASSERT_VIOLATION(do_write(p, SYS_HEAP_KASAN_GRANULE, 0xA2));
	ops->free(p);
}

/* alloc(5): bytes 0..ceil(5/GRANULE)*GRANULE-1 accessible; next byte poisoned. */
static void run_granule_mid_slot(struct heap_ops *ops)
{
	size_t slots = DIV_ROUND_UP(5U, SYS_HEAP_KASAN_GRANULE);
	size_t accessible_end = slots * SYS_HEAP_KASAN_GRANULE - 1;

	uint8_t *p = ops->alloc(5);

	zassert_not_null(p, "alloc(5) failed");
	do_write(p, accessible_end, 0xB1);      /* must not trigger */
	ASSERT_VIOLATION(do_write(p, accessible_end + 1, 0xB2));
	ops->free(p);
}

/* check_write_range early-return for size==0 must not be removed. */
static void run_bulk_zero_size(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memset(p, 0, 0);
	do_memcpy(p, g_src_bytes, 0);
	do_memmove(p, g_src_bytes, 0);
	ops->free(p);
}

/* alloc(64) exercises the full-byte path in shadow_clear. */
static void run_large_alloc_boundary(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(64);

	zassert_not_null(p, "alloc(64) failed");
	for (int i = 0; i < 64; i++) {
		do_write(p, i, (uint8_t)i);
	}
	ASSERT_VIOLATION(do_write(p, 64, 0xEF));
	ops->free(p);
}

/* Write past block A while block B is live - inter-chunk region must be poisoned. */
static void run_adjacent_allocs_overflow(struct heap_ops *ops)
{
	uint8_t *a = ops->alloc(16);
	uint8_t *b = ops->alloc(16);

	zassert_not_null(a, "alloc a failed");
	zassert_not_null(b, "alloc b failed");
	ASSERT_VIOLATION(do_write(a, 16, 0xFF));
	ops->free(a);
	ops->free(b);
}

/*
 * Two live allocations A and B: valid writes to both must not trigger ASAN.
 * Guards against shadow interference between adjacent blocks.
 */
static void run_adjacent_allocs_no_false_positive(struct heap_ops *ops)
{
	uint8_t *a = ops->alloc(16);
	uint8_t *b = ops->alloc(16);

	zassert_not_null(a, "alloc a failed");
	zassert_not_null(b, "alloc b failed");
	for (int i = 0; i < 16; i++) {
		do_write(a, i, (uint8_t)i);
		do_write(b, i, (uint8_t)(i + 16));
	}
	ops->free(a);
	ops->free(b);
}

/*
 * memcpy FROM heap TO stack: dst is a stack buffer, src is heap.
 * ASAN checks only the write destination; a stack dst must never fire.
 */
static void run_memcpy_heap_src_stack_dst_no_false_positive(struct heap_ops *ops)
{
	uint8_t *heap_src = ops->alloc(16);

	zassert_not_null(heap_src, "alloc failed");
	do_memset(heap_src, 0xAB, 16);

	uint8_t stack_dst[16];

	do_memcpy(stack_dst, heap_src, 16);
	ops->free(heap_src);
}

/* Freed chunk must be poisoned; re-alloc of the same chunk must unpoison it. */
static void run_use_after_free_then_realloc(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ops->free(p);
	ASSERT_VIOLATION(do_write(p, 0, 0xAA));

	uint8_t *q = ops->alloc(16);

	zassert_not_null(q, "re-alloc failed");
	for (int i = 0; i < 16; i++) {
		do_write(q, i, (uint8_t)i);
	}
	ops->free(q);
}

static uint8_t g_non_heap_buf[32];

static void run_non_heap_no_false_positive(struct heap_ops *ops)
{
	ARG_UNUSED(ops);
	uint8_t stack_buf[32];
	volatile uint8_t *vs = stack_buf;
	volatile uint8_t *vg = g_non_heap_buf;

	for (int i = 0; i < 32; i++) {
		vs[i] = (uint8_t)i;
		vg[i] = (uint8_t)i;
	}
}

static void run_granule_tail_use_after_free(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(1);
	size_t tail = SYS_HEAP_KASAN_GRANULE - 1; /* last granule-tail byte */

	zassert_not_null(p, "alloc(1) failed");
	do_write(p, tail, 0xAA);             /* must not trigger before free */
	ops->free(p);
	ASSERT_VIOLATION(do_write(p, tail, 0xBB)); /* tail must be poisoned after free */
}

static void run_free_neighbor_no_false_positive(struct heap_ops *ops)
{
	uint8_t *a = ops->alloc(16);
	uint8_t *b = ops->alloc(16);

	zassert_not_null(a, "alloc a failed");
	zassert_not_null(b, "alloc b failed");
	ops->free(a);
	for (int i = 0; i < 16; i++) {
		do_write(b, i, (uint8_t)i); /* B must stay accessible after A is freed */
	}
	ops->free(b);
}

#if CONFIG_SYS_HEAP_KASAN_GRANULE == 1
/* GRANULE_1: byte-precise shadow catches intra-granule tail writes invisible at GRANULE_4. */

/* alloc(1) + write p[1]: byte 1 is a separate slot, poisoned. */
static void run_granule1_tail_overflow_alloc1(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(1);

	zassert_not_null(p, "alloc(1) failed");
	ASSERT_VIOLATION(do_write(p, 1, 0xAB));
	ops->free(p);
}

static void run_granule1_tail_overflow_alloc3(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(3);

	zassert_not_null(p, "alloc(3) failed");
	ASSERT_VIOLATION(do_write(p, 3, 0xCD));
	ops->free(p);
}

static void run_granule1_memset_tail_overflow(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(5);

	zassert_not_null(p, "alloc(5) failed");
	ASSERT_VIOLATION(do_memset(p, 0xEF, 6));
	ops->free(p);
}

static void run_granule1_store8_past_alloc1(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(1);

	zassert_not_null(p, "alloc(1) failed");
	ASSERT_VIOLATION(do_write8(p, 0, 0xDEADC0DEULL));
	ops->free(p);
}

static void run_granule1_realloc_shrink_tail(struct heap_ops *ops)
{
	uint8_t *p = ops->alloc(8);

	zassert_not_null(p, "alloc(8) failed");

	uint8_t *q = ops->realloc(p, 5);

	zassert_not_null(q, "realloc(5) failed");
	do_write(q, 4, 0xAA);                   /* byte 4: last valid byte */
	ASSERT_VIOLATION(do_write(q, 5, 0xBB)); /* byte 5: re-poisoned by realloc */
	ops->free(q);
}

#endif /* CONFIG_SYS_HEAP_KASAN_GRANULE == 1 */

static void run_strcpy_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_strcpy(p, g_str_overflow));
	ops->free(p);
}

static void run_strcpy_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_strcpy(p, g_str_exact);
	ops->free(p);
}

static void run_strcpy_use_after_free(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ops->free(p);
	ASSERT_VIOLATION(do_strcpy(p, g_str_exact));
}

static void run_strncpy_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_strncpy(p, g_str_overflow, 17));
	ops->free(p);
}

static void run_strncpy_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_strncpy(p, g_str_overflow, 16);
	ops->free(p);
}

static void run_strcat_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memcpy(p, g_str_half, 9); /* p = "abcdefgh\0" */
	ASSERT_VIOLATION(do_strcat(p, g_str_append_overflow));
	ops->free(p);
}

static void run_strcat_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memcpy(p, g_str_half, 9);
	do_strcat(p, g_str_append_exact);
	ops->free(p);
}

static void run_strncat_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memcpy(p, g_str_half, 9);
	/* n=8: check_write_range(p+8, 9) -> p[8..16] overflows */
	ASSERT_VIOLATION(do_strncat(p, g_str_overflow, 8));
	ops->free(p);
}

static void run_strncat_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memcpy(p, g_str_half, 9);
	/* n=7: check_write_range(p+8, 8) -> p[8..15] exact */
	do_strncat(p, g_str_overflow, 7);
	ops->free(p);
}

static void run_snprintf_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_snprintf(p, 17, g_str_overflow));
	ops->free(p);
}

static void run_snprintf_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_snprintf(p, 16, g_str_overflow);
	ops->free(p);
}

/* str operations on stack must not trigger ASAN. */
static void run_str_non_heap_no_false_positive(struct heap_ops *ops)
{
	ARG_UNUSED(ops);
	char stack_buf[64];

	do_strcpy(stack_buf, g_str_exact);
	do_strcat(stack_buf, " ok");
	do_snprintf(stack_buf, sizeof(stack_buf), g_str_exact);

#if defined(CONFIG_SYS_HEAP_KASAN_EXTENSIONS)
	static const char fgets_src[] = "hi\n";

	do_stpcpy(stack_buf, g_str_exact);
	do_stpncpy(stack_buf, g_str_exact, sizeof(stack_buf));
	do_memccpy(stack_buf, g_src_bytes, 0, sizeof(stack_buf));
	do_mempcpy(stack_buf, g_src_bytes, 16);
	FILE *f = fmemopen((void *)fgets_src, sizeof(fgets_src), "r");

	if (f != NULL) {
		do_fgets(stack_buf, sizeof(stack_buf), f);
		fclose(f);
	}
#endif
}

static void run_zero_size_str_ops_no_false_positive(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_strncpy(p, g_str_overflow, 0); /* n=0: check(p,0) -> skip */
#if defined(CONFIG_SYS_HEAP_KASAN_EXTENSIONS)
	do_strlcpy(p, g_str_overflow, 0); /* siz=0: check(p,0) -> skip */
#endif
	do_snprintf(p, 0, g_str_overflow); /* n=0: check(p,0) -> skip */
	ops->free(p);
}

static void run_strncat_conservative_check(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memcpy(p, g_str_half, 9); /* p = "abcdefgh\0" */
	/* n=8: check(p+8, 9) -> p[8..16] -> p[16] poisoned, even though
	 * strncat("X", 8) would only write 2 bytes.
	 */
	ASSERT_VIOLATION(do_strncat(p, "X", 8));
	ops->free(p);
}

#if defined(CONFIG_SYS_HEAP_KASAN_EXTENSIONS)

static void run_strlcpy_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_strlcpy(p, g_str_overflow, 17));
	ops->free(p);
}

static void run_strlcpy_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_strlcpy(p, g_str_overflow, 16);
	ops->free(p);
}

static void run_strlcat_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memcpy(p, g_str_half, 9);
	ASSERT_VIOLATION(do_strlcat(p, g_str_overflow, 17));
	ops->free(p);
}

static void run_strlcat_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memcpy(p, g_str_half, 9);
	do_strlcat(p, g_str_overflow, 16);
	ops->free(p);
}

static void run_stpcpy_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_stpcpy(p, g_str_overflow));
	ops->free(p);
}

static void run_stpcpy_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_stpcpy(p, g_str_exact);
	ops->free(p);
}

static void run_stpncpy_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_stpncpy(p, g_str_overflow, 17));
	ops->free(p);
}

static void run_stpncpy_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_stpncpy(p, g_str_overflow, 16);
	ops->free(p);
}

static void run_memccpy_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	/* n=17: __asan_memccpy checks 17 bytes at dst before copying. */
	ASSERT_VIOLATION(do_memccpy(p, g_src_bytes, 0, 17));
	ops->free(p);
}

static void run_memccpy_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_memccpy(p, g_src_bytes, 0, 16);
	ops->free(p);
}

static void run_mempcpy_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_mempcpy(p, g_src_bytes, 17));
	ops->free(p);
}

static void run_mempcpy_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_mempcpy(p, g_src_bytes, 16);
	ops->free(p);
}

static void run_fgets_overflow(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	ASSERT_VIOLATION(do_fgets(p, 17, stdin));
	ops->free(p);
}

static void run_fgets_exact(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");

	static const char src[] = "hello\n";
	FILE *f = fmemopen((void *)src, sizeof(src), "r");

	if (f == NULL) {
		ops->free(p);
		ztest_test_skip();
		return;
	}

	do_fgets(p, 16, f);
	fclose(f);
	ops->free(p);
}

static void run_ext_non_heap_no_false_positive(struct heap_ops *ops)
{
	ARG_UNUSED(ops);
	char stack_buf[64];
	static const char fgets_src[] = "hi\n";

	do_stpcpy(stack_buf, g_str_exact);
	do_stpncpy(stack_buf, g_str_exact, sizeof(stack_buf));
	do_memccpy(stack_buf, g_src_bytes, 0, sizeof(stack_buf));
	do_mempcpy(stack_buf, g_src_bytes, 16);

	FILE *f = fmemopen((void *)fgets_src, sizeof(fgets_src), "r");

	if (f != NULL) {
		do_fgets(stack_buf, sizeof(stack_buf), f);
		fclose(f);
	}
}

static void run_zero_size_ext_ops_no_false_positive(struct heap_ops *ops)
{
	char *p = ops->alloc(16);

	zassert_not_null(p, "alloc failed");
	do_strlcpy(p, g_str_overflow, 0); /* siz=0: check(p,0) -> skip */
	ops->free(p);
}

#endif /* CONFIG_SYS_HEAP_KASAN_EXTENSIONS */

#define ASAN_COMMON_TESTS(suite)                                                       \
	ZTEST_F(suite, test_normal)                                                    \
		{ run_normal(&fixture->ops); }                                         \
	ZTEST_F(suite, test_overflow)                                                  \
		{ run_overflow(&fixture->ops); }                                       \
	ZTEST_F(suite, test_overflow_no_false_positive)                                \
		{ run_overflow_no_false_positive(&fixture->ops); }                     \
	ZTEST_F(suite, test_overflow_8byte)                                            \
		{ run_overflow_8byte(&fixture->ops); }                                 \
	ZTEST_F(suite, test_overflow_8byte_no_false_positive)                          \
		{ run_overflow_8byte_no_false_positive(&fixture->ops); }               \
	ZTEST_F(suite, test_overflow_8byte_at_slot7)                                   \
		{ run_overflow_8byte_at_slot7(&fixture->ops); }                        \
	ZTEST_F(suite, test_overflow_8byte_at_slot7_no_false_positive)                 \
		{ run_overflow_8byte_at_slot7_no_false_positive(&fixture->ops); }      \
	ZTEST_F(suite, test_overflow_8byte_at_slot31)                                  \
		{ run_overflow_8byte_at_slot31(&fixture->ops); }                       \
	ZTEST_F(suite, test_overflow_8byte_at_slot31_no_false_positive)                \
		{ run_overflow_8byte_at_slot31_no_false_positive(&fixture->ops); }     \
	ZTEST_F(suite, test_overflow_misaligned_granule_span)                          \
		{ run_overflow_misaligned_granule_span(&fixture->ops); }               \
	ZTEST_F(suite, test_overflow_misaligned_granule_span_no_false_positive)        \
		{ run_overflow_misaligned_granule_span_no_false_positive(&fixture->ops); } \
	ZTEST_F(suite, test_underflow)                                                 \
		{ run_underflow(&fixture->ops); }                                      \
	ZTEST_F(suite, test_use_after_free)                                            \
		{ run_use_after_free(&fixture->ops); }                                 \
	ZTEST_F(suite, test_use_after_free_memset)                                     \
		{ run_use_after_free_memset(&fixture->ops); }                          \
	ZTEST_F(suite, test_use_after_free_memcpy)                                     \
		{ run_use_after_free_memcpy(&fixture->ops); }                          \
	ZTEST_F(suite, test_use_after_free_interior_ptr)                               \
		{ run_use_after_free_interior_ptr(&fixture->ops); }                    \
	ZTEST_F(suite, test_realloc_shrink_suffix)                                     \
		{ run_realloc_shrink_suffix(&fixture->ops); }                          \
	ZTEST_F(suite, test_realloc_shrink_no_false_positive)                          \
		{ run_realloc_shrink_no_false_positive(&fixture->ops); }               \
	ZTEST_F(suite, test_realloc_expand)                                            \
		{ run_realloc_expand(&fixture->ops); }                                 \
	ZTEST_F(suite, test_realloc_expand_no_false_positive)                          \
		{ run_realloc_expand_no_false_positive(&fixture->ops); }               \
	ZTEST_F(suite, test_realloc_free_use_after)                                    \
		{ run_realloc_free_use_after(&fixture->ops); }                         \
	ZTEST_F(suite, test_use_after_realloc_moved)                                   \
		{ run_use_after_realloc_moved(&fixture->ops); }                        \
	ZTEST_F(suite, test_realloc_shrink_to_one_granule_boundary)                    \
		{ run_realloc_shrink_to_one_granule_boundary(&fixture->ops); }         \
	ZTEST_F(suite, test_memset_overflow)                                           \
		{ run_memset_overflow(&fixture->ops); }                                \
	ZTEST_F(suite, test_memset_exact)                                              \
		{ run_memset_exact(&fixture->ops); }                                   \
	ZTEST_F(suite, test_memcpy_overflow)                                           \
		{ run_memcpy_overflow(&fixture->ops); }                                \
	ZTEST_F(suite, test_memcpy_exact)                                              \
		{ run_memcpy_exact(&fixture->ops); }                                   \
	ZTEST_F(suite, test_memmove_overflow)                                          \
		{ run_memmove_overflow(&fixture->ops); }                               \
	ZTEST_F(suite, test_memmove_exact)                                             \
		{ run_memmove_exact(&fixture->ops); }                                  \
	ZTEST_F(suite, test_memset_mid_overflow)                                       \
		{ run_memset_mid_overflow(&fixture->ops); }                            \
	ZTEST_F(suite, test_memset_mid_exact)                                          \
		{ run_memset_mid_exact(&fixture->ops); }                               \
	ZTEST_F(suite, test_granule_single_slot)                                       \
		{ run_granule_single_slot(&fixture->ops); }                            \
	ZTEST_F(suite, test_granule_mid_slot)                                          \
		{ run_granule_mid_slot(&fixture->ops); }                               \
	ZTEST_F(suite, test_bulk_zero_size)                                            \
		{ run_bulk_zero_size(&fixture->ops); }                                 \
	ZTEST_F(suite, test_large_alloc_boundary)                                      \
		{ run_large_alloc_boundary(&fixture->ops); }                           \
	ZTEST_F(suite, test_adjacent_allocs_overflow)                                  \
		{ run_adjacent_allocs_overflow(&fixture->ops); }                       \
	ZTEST_F(suite, test_adjacent_allocs_no_false_positive)                         \
		{ run_adjacent_allocs_no_false_positive(&fixture->ops); }              \
	ZTEST_F(suite, test_memcpy_heap_src_stack_dst_no_false_positive)               \
		{ run_memcpy_heap_src_stack_dst_no_false_positive(&fixture->ops); }    \
	ZTEST_F(suite, test_use_after_free_then_realloc)                               \
		{ run_use_after_free_then_realloc(&fixture->ops); }                    \
	ZTEST_F(suite, test_overflow_memset_full_bundle)                               \
		{ run_overflow_memset_full_bundle(&fixture->ops); }                    \
	ZTEST_F(suite, test_overflow_memset_two_bundles)                               \
		{ run_overflow_memset_two_bundles(&fixture->ops); }                    \
	ZTEST_F(suite, test_non_heap_no_false_positive)                                \
		{ run_non_heap_no_false_positive(&fixture->ops); }                     \
	ZTEST_F(suite, test_granule_tail_use_after_free)                               \
		{ run_granule_tail_use_after_free(&fixture->ops); }                    \
	ZTEST_F(suite, test_free_neighbor_no_false_positive)                           \
		{ run_free_neighbor_no_false_positive(&fixture->ops); }

#define ASAN_STR_TESTS(suite)                                                          \
	ZTEST_F(suite, test_strcpy_overflow)                                           \
		{ run_strcpy_overflow(&fixture->ops); }                                \
	ZTEST_F(suite, test_strcpy_exact)                                              \
		{ run_strcpy_exact(&fixture->ops); }                                   \
	ZTEST_F(suite, test_strcpy_use_after_free)                                     \
		{ run_strcpy_use_after_free(&fixture->ops); }                          \
	ZTEST_F(suite, test_strncpy_overflow)                                          \
		{ run_strncpy_overflow(&fixture->ops); }                               \
	ZTEST_F(suite, test_strncpy_exact)                                             \
		{ run_strncpy_exact(&fixture->ops); }                                  \
	ZTEST_F(suite, test_strcat_overflow)                                           \
		{ run_strcat_overflow(&fixture->ops); }                                \
	ZTEST_F(suite, test_strcat_exact)                                              \
		{ run_strcat_exact(&fixture->ops); }                                   \
	ZTEST_F(suite, test_strncat_overflow)                                          \
		{ run_strncat_overflow(&fixture->ops); }                               \
	ZTEST_F(suite, test_strncat_exact)                                             \
		{ run_strncat_exact(&fixture->ops); }                                  \
	ZTEST_F(suite, test_snprintf_overflow)                                         \
		{ run_snprintf_overflow(&fixture->ops); }                              \
	ZTEST_F(suite, test_snprintf_exact)                                            \
		{ run_snprintf_exact(&fixture->ops); }                                 \
	ZTEST_F(suite, test_zero_size_str_ops_no_false_positive)                       \
		{ run_zero_size_str_ops_no_false_positive(&fixture->ops); }            \
	ZTEST_F(suite, test_strncat_conservative_check)                                \
		{ run_strncat_conservative_check(&fixture->ops); }                     \
	ZTEST_F(suite, test_str_non_heap_no_false_positive)                            \
		{ run_str_non_heap_no_false_positive(&fixture->ops); }

#if defined(CONFIG_SYS_HEAP_KASAN_EXTENSIONS)
#define ASAN_EXT_TESTS(suite)                                                          \
	ZTEST_F(suite, test_strlcpy_overflow)                                          \
		{ run_strlcpy_overflow(&fixture->ops); }                               \
	ZTEST_F(suite, test_strlcpy_exact)                                             \
		{ run_strlcpy_exact(&fixture->ops); }                                  \
	ZTEST_F(suite, test_strlcat_overflow)                                          \
		{ run_strlcat_overflow(&fixture->ops); }                               \
	ZTEST_F(suite, test_strlcat_exact)                                             \
		{ run_strlcat_exact(&fixture->ops); }                                  \
	ZTEST_F(suite, test_stpcpy_overflow)                                           \
		{ run_stpcpy_overflow(&fixture->ops); }                                \
	ZTEST_F(suite, test_stpcpy_exact)                                              \
		{ run_stpcpy_exact(&fixture->ops); }                                   \
	ZTEST_F(suite, test_stpncpy_overflow)                                          \
		{ run_stpncpy_overflow(&fixture->ops); }                               \
	ZTEST_F(suite, test_stpncpy_exact)                                             \
		{ run_stpncpy_exact(&fixture->ops); }                                  \
	ZTEST_F(suite, test_memccpy_overflow)                                          \
		{ run_memccpy_overflow(&fixture->ops); }                               \
	ZTEST_F(suite, test_memccpy_exact)                                             \
		{ run_memccpy_exact(&fixture->ops); }                                  \
	ZTEST_F(suite, test_mempcpy_overflow)                                          \
		{ run_mempcpy_overflow(&fixture->ops); }                               \
	ZTEST_F(suite, test_mempcpy_exact)                                             \
		{ run_mempcpy_exact(&fixture->ops); }                                  \
	ZTEST_F(suite, test_fgets_overflow)                                            \
		{ run_fgets_overflow(&fixture->ops); }                                 \
	ZTEST_F(suite, test_fgets_exact)                                               \
		{ run_fgets_exact(&fixture->ops); }                                    \
	ZTEST_F(suite, test_ext_non_heap_no_false_positive)                            \
		{ run_ext_non_heap_no_false_positive(&fixture->ops); }                 \
	ZTEST_F(suite, test_zero_size_ext_ops_no_false_positive)                       \
		{ run_zero_size_ext_ops_no_false_positive(&fixture->ops); }
#else
#define ASAN_EXT_TESTS(suite)
#endif

#if CONFIG_SYS_HEAP_KASAN_GRANULE == 1
#define ASAN_GRANULE1_TESTS(suite)                                                         \
	ZTEST_F(suite, test_granule1_tail_overflow_alloc1)                                 \
		{ run_granule1_tail_overflow_alloc1(&fixture->ops); }                      \
	ZTEST_F(suite, test_granule1_tail_overflow_alloc3)                                 \
		{ run_granule1_tail_overflow_alloc3(&fixture->ops); }                      \
	ZTEST_F(suite, test_granule1_memset_tail_overflow)                                 \
		{ run_granule1_memset_tail_overflow(&fixture->ops); }                      \
	ZTEST_F(suite, test_granule1_store8_past_alloc1)                                   \
		{ run_granule1_store8_past_alloc1(&fixture->ops); }                        \
	ZTEST_F(suite, test_granule1_realloc_shrink_tail)                                  \
		{ run_granule1_realloc_shrink_tail(&fixture->ops); }
#else
#define ASAN_GRANULE1_TESTS(suite)
#endif

#if defined(CONFIG_SYS_HEAP_KASAN_MALLOC)
ZTEST_SUITE(asan_malloc, NULL, malloc_setup, malloc_before, NULL, NULL);
ASAN_COMMON_TESTS(asan_malloc)
ASAN_STR_TESTS(asan_malloc)
ASAN_EXT_TESTS(asan_malloc)
ASAN_GRANULE1_TESTS(asan_malloc)
#endif

#if defined(CONFIG_SYS_HEAP_KASAN_SYSTEM)
ZTEST_SUITE(asan_sys_heap, NULL, sys_heap_setup, sys_heap_before, NULL, NULL);
ASAN_COMMON_TESTS(asan_sys_heap)
ASAN_STR_TESTS(asan_sys_heap)
ASAN_EXT_TESTS(asan_sys_heap)
ASAN_GRANULE1_TESTS(asan_sys_heap)
#endif

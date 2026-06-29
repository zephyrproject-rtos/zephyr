/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_IRQ_OFFLOAD
#include <zephyr/irq_offload.h>
#endif
#include "heap.h"

LOG_MODULE_REGISTER(lib_heap_stats_log, LOG_LEVEL_DBG);

#define HEAP_SZ  2048
#define STACK_SZ 1024

static uint8_t heap_mem[HEAP_SZ] __aligned(8);

#ifdef CONFIG_SYS_HEAP_THREAD_STATS

static struct z_heap_thread_stat *find_stat(struct sys_heap *heap,
					    struct k_thread *thread)
{
	struct z_heap *h = heap->heap;

	if (h->stats == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < h->stats->num_threads; i++) {
		if (h->stats->threads[i].thread == thread) {
			return &h->stats->threads[i];
		}
	}
	return NULL;
}

#endif /* CONFIG_SYS_HEAP_THREAD_STATS */

/* alloc increments total_alloc; free returns it to zero */
ZTEST(lib_heap_stats_log, test_alloc_free_tracked)
{
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
	struct sys_heap heap;
	struct k_thread *self = k_current_get();
	struct z_heap_thread_stat *stat;
	void *p;
	size_t usable;

	sys_heap_init(&heap, heap_mem, HEAP_SZ);

	p = sys_heap_alloc(&heap, 64);
	zassert_not_null(p, "allocation must succeed");

	usable = sys_heap_usable_size(&heap, p);
	stat = find_stat(&heap, self);
	zassert_not_null(stat, "current thread must appear in stats");
	zassert_equal(stat->total_alloc, usable,
		      "total_alloc (%zu) must equal usable size (%zu)",
		      stat->total_alloc, usable);

	sys_heap_free(&heap, p);
	zassert_equal(stat->total_alloc, 0,
		      "total_alloc must be 0 after free");
	zassert_true(sys_heap_validate(&heap), "heap must be valid");
#else
	ztest_test_skip();
#endif
}

/* ISR allocations go to isr_alloc_bytes, not the per-thread table */
#if defined(CONFIG_SYS_HEAP_THREAD_STATS) && defined(CONFIG_IRQ_OFFLOAD)
static void *isr_alloc_ptr;
static struct sys_heap *isr_heap_ptr;
static void isr_alloc_fn(const void *param)
{
	ARG_UNUSED(param);

	isr_alloc_ptr = sys_heap_alloc(isr_heap_ptr, 64);
}
#endif

ZTEST(lib_heap_stats_log, test_isr_alloc_tracked)
{
#if defined(CONFIG_SYS_HEAP_THREAD_STATS) && defined(CONFIG_IRQ_OFFLOAD)
	struct sys_heap heap;
	struct k_thread *self = k_current_get();
	struct z_heap_thread_stat *stat;
	size_t usable;
	void *p;

	sys_heap_init(&heap, heap_mem, HEAP_SZ);
	isr_alloc_ptr = NULL;
	isr_heap_ptr = &heap;

	irq_offload(isr_alloc_fn, NULL);
	zassert_not_null(isr_alloc_ptr, "ISR alloc must succeed");

	/* ISR alloc must not create a thread entry */
	zassert_equal(heap.heap->stats->num_threads, 0,
		      "ISR alloc must not create a thread entry");

	/* isr_alloc_bytes must reflect the outstanding ISR allocation */
	usable = sys_heap_usable_size(&heap, isr_alloc_ptr);
	zassert_equal(heap.heap->stats->isr_alloc_bytes,
		      usable, "isr_alloc_bytes must equal usable size");

	/* Free from thread context; isr_alloc_bytes must return to zero */
	sys_heap_free(&heap, isr_alloc_ptr);
	zassert_equal(heap.heap->stats->isr_alloc_bytes,
		      0, "isr_alloc_bytes must reach 0 after free");
	zassert_true(sys_heap_validate(&heap), "heap invalid after ISR free");

	/* A regular thread alloc afterwards must still be tracked normally */
	p = sys_heap_alloc(&heap, 32);
	zassert_not_null(p, "thread alloc after ISR test failed");
	stat = find_stat(&heap, self);
	zassert_not_null(stat, "thread alloc must be tracked after ISR alloc");
	zassert_equal(stat->total_alloc, sys_heap_usable_size(&heap, p),
		      "thread alloc must equal usable size");
	sys_heap_free(&heap, p);
#else
	ztest_test_skip();
#endif
}

/* Thread name is captured on first alloc and refreshed if set later */
ZTEST(lib_heap_stats_log, test_thread_name_captured)
{
#if defined(CONFIG_SYS_HEAP_THREAD_STATS) && defined(CONFIG_THREAD_NAME)
	struct sys_heap heap;
	struct k_thread *self = k_current_get();
	struct z_heap_thread_stat *stat;
	void *p;

	/* First alloc with no name -- entry created with empty name */
	k_thread_name_set(self, "");
	sys_heap_init(&heap, heap_mem, HEAP_SZ);

	p = sys_heap_alloc(&heap, 8);
	zassert_not_null(p, "alloc must succeed");
	stat = find_stat(&heap, self);
	zassert_not_null(stat, "thread must be in stats");
	zassert_equal(stat->name[0], '\0', "name must be empty before it is set");

	/* Set name then alloc again -- entry must be refreshed */
	k_thread_name_set(self, "ztest_main");
	sys_heap_free(&heap, p);

	p = sys_heap_alloc(&heap, 8);
	zassert_not_null(p, "second alloc must succeed");
	zassert_str_equal(stat->name, "ztest_main",
			  "name must be refreshed on next alloc; got '%s'",
			  stat->name);

	sys_heap_free(&heap, p);
	k_thread_name_set(self, "ztest_main");
#else
	ztest_test_skip();
#endif
}

/* caller pointer is stored and readable via sys_heap_get_caller() */
ZTEST(lib_heap_stats_log, test_caller_pointer_captured)
{
#ifdef CONFIG_SYS_HEAP_CALLER_POINTER
	struct sys_heap heap;
	void *p;

	sys_heap_init(&heap, heap_mem, HEAP_SZ);

	p = sys_heap_alloc(&heap, 32);
	zassert_not_null(p, "allocation must succeed");
	zassert_not_null(sys_heap_get_caller(&heap, p),
			 "caller pointer must be non-NULL after allocation");

	sys_heap_free(&heap, p);
#else
	ztest_test_skip();
#endif
}

/* Two threads each get their own stats entry */
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
static uint8_t heap2_mem[HEAP_SZ] __aligned(8);
static K_THREAD_STACK_DEFINE(worker_stack_a, STACK_SZ);
static K_THREAD_STACK_DEFINE(worker_stack_b, STACK_SZ);
static struct k_thread worker_tcb_a;
static struct k_thread worker_tcb_b;
static K_SEM_DEFINE(worker_done, 0, 1);
static struct sys_heap thread_heap;
static void *worker_a_ptr;
static void *worker_b_ptr;

static void worker_a_fn(void *h, void *done, void *unused)
{
	ARG_UNUSED(unused);
	worker_a_ptr = sys_heap_alloc((struct sys_heap *)h, 48);
	k_sem_give((struct k_sem *)done);
}

static void worker_b_fn(void *h, void *done, void *unused)
{
	ARG_UNUSED(unused);
	worker_b_ptr = sys_heap_alloc((struct sys_heap *)h, 80);
	k_sem_give((struct k_sem *)done);
}
#endif

ZTEST(lib_heap_stats_log, test_two_threads_tracked_separately)
{
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
	struct z_heap_thread_stat *sa, *sb;

	sys_heap_init(&thread_heap, heap2_mem, HEAP_SZ);
	worker_a_ptr = NULL;
	worker_b_ptr = NULL;

	k_thread_create(&worker_tcb_a, worker_stack_a, STACK_SZ,
			worker_a_fn, &thread_heap, &worker_done, NULL,
			K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_sem_take(&worker_done, K_FOREVER);
	k_thread_join(&worker_tcb_a, K_FOREVER);

	k_thread_create(&worker_tcb_b, worker_stack_b, STACK_SZ,
			worker_b_fn, &thread_heap, &worker_done, NULL,
			K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_sem_take(&worker_done, K_FOREVER);
	k_thread_join(&worker_tcb_b, K_FOREVER);

	zassert_not_null(worker_a_ptr, "worker_a allocation failed");
	zassert_not_null(worker_b_ptr, "worker_b allocation failed");

	zassert_equal(thread_heap.heap->stats->num_threads,
		      2, "exactly 2 threads must be tracked");

	sa = find_stat(&thread_heap, &worker_tcb_a);
	sb = find_stat(&thread_heap, &worker_tcb_b);
	zassert_not_null(sa, "worker_a must have its own stats entry");
	zassert_not_null(sb, "worker_b must have its own stats entry");
	zassert_equal(sa->total_alloc,
		      sys_heap_usable_size(&thread_heap, worker_a_ptr),
		      "worker_a total_alloc mismatch");
	zassert_equal(sb->total_alloc,
		      sys_heap_usable_size(&thread_heap, worker_b_ptr),
		      "worker_b total_alloc mismatch");

	zassert_true(sys_heap_validate(&thread_heap),
		     "heap invalid after threaded ops");
	sys_heap_free(&thread_heap, worker_a_ptr);
	sys_heap_free(&thread_heap, worker_b_ptr);
#else
	ztest_test_skip();
#endif
}

/* in-place realloc (shrink and expand) updates stats correctly */
ZTEST(lib_heap_stats_log, test_inplace_realloc_updates_stats)
{
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
	struct sys_heap heap;
	struct k_thread *self = k_current_get();
	struct z_heap_thread_stat *stat;
	void *p, *p2, *p_resized;
	size_t usable_after;

	sys_heap_init(&heap, heap_mem, HEAP_SZ);

	/* Shrink in place */
	p = sys_heap_alloc(&heap, 128);
	zassert_not_null(p, "initial alloc failed");
	p2 = sys_heap_alloc(&heap, 8);
	zassert_not_null(p2, "anchor alloc failed");

	p_resized = sys_heap_realloc(&heap, p, 32);
	zassert_true(sys_heap_validate(&heap), "heap invalid after shrink");

	if (p_resized == p) {
		stat = find_stat(&heap, self);
		zassert_not_null(stat, "thread must be in stats after shrink");
		usable_after = sys_heap_usable_size(&heap, p_resized)
			     + sys_heap_usable_size(&heap, p2);
		zassert_equal(stat->total_alloc, usable_after,
			      "total_alloc must equal shrunk+anchor usable bytes");
	}

	sys_heap_free(&heap, p_resized);
	sys_heap_free(&heap, p2);

	/* Expand in place */
	p = sys_heap_alloc(&heap, 64);
	zassert_not_null(p, "initial alloc failed");

	p_resized = sys_heap_realloc(&heap, p, 128);
	zassert_not_null(p_resized, "expand realloc failed");
	zassert_true(sys_heap_validate(&heap), "heap invalid after expand");

	stat = find_stat(&heap, self);
	zassert_not_null(stat, "thread must be in stats after expand");
	if (p_resized == p) {
		zassert_equal(stat->total_alloc,
			      sys_heap_usable_size(&heap, p_resized),
			      "total_alloc must equal expanded usable size");
	}

	sys_heap_free(&heap, p_resized);
	zassert_equal(stat->total_alloc, 0, "total_alloc must be 0 after free");
	zassert_true(sys_heap_validate(&heap), "heap invalid after cleanup");
#else
	ztest_test_skip();
#endif
}

/* non-in-place realloc (alloc+copy+free) tracks new allocation */
ZTEST(lib_heap_stats_log, test_non_inplace_realloc_tracked)
{
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
	struct sys_heap heap;
	struct k_thread *self = k_current_get();
	struct z_heap_thread_stat *stat;
	void *p1, *p2, *p3;

	sys_heap_init(&heap, heap_mem, HEAP_SZ);

	p1 = sys_heap_alloc(&heap, 64);
	p2 = sys_heap_alloc(&heap, 64);
	zassert_not_null(p1, "p1 alloc failed");
	zassert_not_null(p2, "p2 alloc failed");

	p3 = sys_heap_realloc(&heap, p1, 256);
	zassert_not_null(p3, "realloc failed");
	zassert_true(sys_heap_validate(&heap), "heap invalid after move-realloc");

	stat = find_stat(&heap, self);
	zassert_not_null(stat, "thread must be in stats");
	zassert_equal(stat->total_alloc,
		      sys_heap_usable_size(&heap, p2) +
		      sys_heap_usable_size(&heap, p3),
		      "total_alloc must be p2+p3 usable bytes");

	sys_heap_free(&heap, p2);
	sys_heap_free(&heap, p3);
	zassert_true(sys_heap_validate(&heap), "heap invalid after cleanup");
#else
	ztest_test_skip();
#endif
}

/* sys_heap_stats_log() does not crash on any heap state */
ZTEST(lib_heap_stats_log, test_stats_log_does_not_crash)
{
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
	struct sys_heap heap;
	void *p;

	sys_heap_init(&heap, heap_mem, HEAP_SZ);

	p = sys_heap_alloc(&heap, 64);
	zassert_not_null(p, "allocation must succeed");

	sys_heap_stats_log(&heap);
	sys_heap_free(&heap, p);
	sys_heap_stats_log(&heap);
	sys_heap_stats_log(NULL);
#else
	ztest_test_skip();
#endif
}

/* thread table caps at MAX_TRACKED_THREADS; overflow goes to counter */
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
#define OVF_WORKERS (CONFIG_SYS_HEAP_STATS_MAX_TRACKED_THREADS + 2)
static K_THREAD_STACK_ARRAY_DEFINE(ovf_stacks, OVF_WORKERS, STACK_SZ);
static struct k_thread ovf_threads[OVF_WORKERS];
static void *ovf_ptrs[OVF_WORKERS];
static struct k_sem ovf_done;
static struct sys_heap ovf_heap;
static uint8_t ovf_heap_mem[HEAP_SZ] __aligned(8);

static void ovf_worker_fn(void *heap_arg, void *idx_arg, void *unused)
{
	ARG_UNUSED(unused);
	ovf_ptrs[POINTER_TO_INT(idx_arg)] =
		sys_heap_alloc((struct sys_heap *)heap_arg, 32);
	k_sem_give(&ovf_done);
}
#endif

ZTEST(lib_heap_stats_log, test_thread_table_caps_with_real_threads)
{
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
	int i;

	sys_heap_init(&ovf_heap, ovf_heap_mem, HEAP_SZ);

	if (ovf_heap.heap->stats == NULL) {
		ztest_test_skip();
	}

	k_sem_init(&ovf_done, 0, OVF_WORKERS);

	/* Spawn one thread at a time so sys_heap access is serialised. */
	for (i = 0; i < OVF_WORKERS; i++) {
		ovf_ptrs[i] = NULL;
		k_thread_create(&ovf_threads[i], ovf_stacks[i], STACK_SZ,
				ovf_worker_fn, &ovf_heap, INT_TO_POINTER(i),
				NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
		k_sem_take(&ovf_done, K_FOREVER);
		k_thread_join(&ovf_threads[i], K_FOREVER);
	}

	zassert_equal(ovf_heap.heap->stats->num_threads,
		      (size_t)CONFIG_SYS_HEAP_STATS_MAX_TRACKED_THREADS,
		      "num_threads must be capped at CONFIG_SYS_HEAP_STATS_MAX_TRACKED_THREADS");

	for (i = 0; i < OVF_WORKERS; i++) {
		if (ovf_ptrs[i] != NULL) {
			sys_heap_free(&ovf_heap, ovf_ptrs[i]);
		}
	}
	zassert_true(sys_heap_validate(&ovf_heap),
		     "heap invalid after cleanup");
#else
	ztest_test_skip();
#endif
}

/* randomised alloc/free stress: total_alloc must equal live usable bytes */
ZTEST(lib_heap_stats_log, test_alloc_stress_no_stats_drift)
{
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
	struct sys_heap heap;
	struct k_thread *self = k_current_get();
	struct z_heap_thread_stat *stat;
	void *ptrs[16] = { NULL };
	size_t live_bytes[16] = { 0 };
	size_t total_live = 0;
	uint32_t seed = 0xdeadbeefU;
	int i;

	sys_heap_init(&heap, heap_mem, HEAP_SZ);

	for (int iter = 0; iter < 500; iter++) {
		seed = seed * 1103515245U + 12345U;
		i = (int)((seed >> 8) & 0xfU);

		if (ptrs[i] != NULL) {
			total_live -= live_bytes[i];
			sys_heap_free(&heap, ptrs[i]);
			ptrs[i] = NULL;
			live_bytes[i] = 0;
		} else {
			seed = seed * 1103515245U + 12345U;
			size_t sz = ((seed >> 16) & 0x7fU) + 1U;

			ptrs[i] = sys_heap_alloc(&heap, sz);
			if (ptrs[i] != NULL) {
				live_bytes[i] = sys_heap_usable_size(&heap,
								     ptrs[i]);
				total_live += live_bytes[i];
			}
		}

		stat = find_stat(&heap, self);
		zassert_not_null(stat, "thread stats must exist");
		zassert_equal(stat->total_alloc, total_live,
			      "drift at iter %d: total_alloc=%zu live=%zu",
			      iter, stat->total_alloc, total_live);
	}

	zassert_true(sys_heap_validate(&heap), "heap invalid after stress");

	for (i = 0; i < (int)ARRAY_SIZE(ptrs); i++) {
		if (ptrs[i] != NULL) {
			sys_heap_free(&heap, ptrs[i]);
		}
	}
	stat = find_stat(&heap, self);
	zassert_equal(stat->total_alloc, 0,
		      "total_alloc must return to 0 after draining");
	zassert_true(sys_heap_validate(&heap), "heap invalid after drain");
#else
	ztest_test_skip();
#endif
}

/*
 * Pool exhaustion: initialising more heaps than CONFIG_SYS_HEAP_STATS_TRACKED_HEAPS
 * must result in at least one heap with a NULL stats pointer, while all heaps
 * continue to allocate and free correctly.
 */
ZTEST(lib_heap_stats_log, test_z_pool_exhaustion_null_stats)
{
#ifdef CONFIG_SYS_HEAP_THREAD_STATS
#define EX_COUNT (CONFIG_SYS_HEAP_STATS_TRACKED_HEAPS + 1)
	static struct sys_heap ex_heaps[EX_COUNT];
	static uint8_t ex_mem[EX_COUNT][512] __aligned(8);
	int null_count = 0;
	int i;

	for (i = 0; i < EX_COUNT; i++) {
		sys_heap_init(&ex_heaps[i], ex_mem[i], sizeof(ex_mem[i]));
	}

	for (i = 0; i < EX_COUNT; i++) {
		if (ex_heaps[i].heap->stats == NULL) {
			null_count++;
		}
	}

	zassert_true(null_count > 0,
		     "at least one heap must have NULL stats after pool exhaustion");

	for (i = 0; i < EX_COUNT; i++) {
		void *p = sys_heap_alloc(&ex_heaps[i], 32);

		zassert_not_null(p, "alloc on heap %d failed", i);
		sys_heap_free(&ex_heaps[i], p);
		zassert_true(sys_heap_validate(&ex_heaps[i]),
			     "heap %d invalid after alloc/free", i);
	}
#undef EX_COUNT
#else
	ztest_test_skip();
#endif
}

ZTEST_SUITE(lib_heap_stats_log, NULL, NULL, NULL, NULL, NULL);

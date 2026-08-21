/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/sys/multi_heap.h>

/* The tracked object is identified by address only and is never dereferenced,
 * so the caller can look for a block it has already released. Addresses are
 * passed as uintptr_t rather than as a pointer so that comparing against freed
 * memory does not trip -Wuse-after-free.
 */
static int count_sem(uintptr_t target)
{
	void *list = _track_list_k_sem;
	int count = 0;

	while (list != NULL) {
		if ((uintptr_t)list == target) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_sem *)list);
	}

	return count;
}

/* A kernel object initialized on the heap is added to a tracking list, and the
 * list has no removal path of its own. Freeing that memory used to leave a
 * dangling node behind, so the next traversal followed a link into memory that
 * had already been reused.
 *
 * This suite keeps every tracked object either static or heap allocated, so the
 * lists stay walkable for the whole run.
 */
ZTEST(obj_tracking_free, test_tracked_object_removed_on_free)
{
	static struct k_sem after_free_sem;
	struct k_sem *heap_sem;
	uintptr_t freed_addr;
	void *filler;

	heap_sem = k_malloc(sizeof(struct k_sem));
	zassert_not_null(heap_sem, "Failed to allocate semaphore");

	k_sem_init(heap_sem, 0, 1);
	zassert_equal(count_sem((uintptr_t)heap_sem), 1, "Heap semaphore was not tracked");

	freed_addr = (uintptr_t)heap_sem;
	k_free(heap_sem);

	/* Reuse the freed memory and scribble over it, so a stale node would
	 * point at a link value that is no longer a valid object.
	 */
	filler = k_malloc(sizeof(struct k_sem));
	zassert_not_null(filler, "Failed to reallocate semaphore memory");
	memset(filler, 0xAA, sizeof(struct k_sem));

	zassert_equal(count_sem(freed_addr), 0, "Freed semaphore is still tracked");

	k_free(filler);

	/* Tracking a new object still has to work after a removal */
	k_sem_init(&after_free_sem, 0, 1);
	zassert_equal(count_sem((uintptr_t)&after_free_sem), 1,
		      "Semaphore tracking broken after a free");
}

/* k_realloc() bypasses k_heap_free() and goes straight to the heap. When the
 * block cannot grow in place it is moved, and the tracked object left behind
 * at the old address has to be dropped by the allocator itself.
 */
ZTEST(obj_tracking_free, test_tracked_object_removed_on_realloc)
{
	struct k_sem *heap_sem;
	uintptr_t old_addr;
	void *block;
	void *moved;

	heap_sem = k_malloc(sizeof(struct k_sem));
	zassert_not_null(heap_sem, "Failed to allocate semaphore");

	/* Occupy the neighbouring memory so the block cannot simply expand */
	block = k_malloc(sizeof(struct k_sem));
	zassert_not_null(block, "Failed to allocate the blocking chunk");

	k_sem_init(heap_sem, 0, 1);
	old_addr = (uintptr_t)heap_sem;
	zassert_equal(count_sem(old_addr), 1, "Heap semaphore was not tracked");

	moved = k_realloc(heap_sem, 512);
	zassert_not_null(moved, "Failed to grow the allocation");

	if ((uintptr_t)moved == old_addr) {
		/* The block grew in place, so the object never moved and is
		 * still live at the same address. Nothing to untrack here.
		 */
		k_free(moved);
		k_free(block);
		ztest_test_skip();
	}

	zassert_equal(count_sem(old_addr), 0, "Semaphore is still tracked after realloc");

	k_free(moved);
	k_free(block);
}

/* libc free() reaches the heap without going through k_free() at all. This
 * only applies when libc allocates from a Zephyr heap; a native libc hands the
 * allocation to the host instead.
 */
ZTEST(obj_tracking_free, test_tracked_object_removed_on_libc_free)
{
	struct k_sem *heap_sem;
	uintptr_t freed_addr;

	if (IS_ENABLED(CONFIG_NATIVE_LIBC)) {
		ztest_test_skip();
	}

	heap_sem = malloc(sizeof(struct k_sem));
	zassert_not_null(heap_sem, "Failed to allocate semaphore");

	k_sem_init(heap_sem, 0, 1);
	zassert_equal(count_sem((uintptr_t)heap_sem), 1, "Heap semaphore was not tracked");

	freed_addr = (uintptr_t)heap_sem;
	free(heap_sem);
	zassert_equal(count_sem(freed_addr), 0, "Semaphore is still tracked after libc free");
}

/* Enough elements that the last one lands far inside the released tail. */
#define SHRINK_SEMS 16

/* Shrinking a block in place splits off the tail and releases it without going
 * through sys_heap_free(), so a tracked object living in that tail has to be
 * dropped by the shrink path itself.
 */
ZTEST(obj_tracking_free, test_tracked_object_removed_on_shrink)
{
	struct k_sem *sems;
	struct k_sem *tail_sem;
	uintptr_t tail_addr;
	void *block;
	void *shrunk;

	/* Typed allocation so every element is correctly aligned for the
	 * object placed in it, whatever the platform requires.
	 */
	sems = k_malloc(SHRINK_SEMS * sizeof(struct k_sem));
	zassert_not_null(sems, "Failed to allocate the block");
	block = sems;

	/* Place a tracked object well inside the region that the shrink is
	 * about to split off and release.
	 */
	tail_sem = &sems[SHRINK_SEMS - 1];
	k_sem_init(tail_sem, 0, 1);
	tail_addr = (uintptr_t)tail_sem;
	zassert_equal(count_sem(tail_addr), 1, "Tail semaphore was not tracked");

	shrunk = k_realloc(block, sizeof(struct k_sem));
	zassert_not_null(shrunk, "Failed to shrink the allocation");

	if ((uintptr_t)shrunk != (uintptr_t)block) {
		/* The allocator moved the block instead of splitting it, so
		 * the tail went through sys_heap_free() rather than the
		 * shrink path this test targets.
		 */
		k_free(shrunk);
		ztest_test_skip();
	}

	zassert_equal(count_sem(tail_addr), 0, "Semaphore in the released tail is still tracked");

	k_free(shrunk);
}

static struct sys_heap heap;

static void *multi_heap_choice(struct sys_multi_heap *mheap, void *cfg, size_t align, size_t size)
{
	ARG_UNUSED(mheap);
	ARG_UNUSED(cfg);

	return sys_heap_aligned_alloc(&heap, align, size);
}

/* sys_multi_heap_free() reaches the backing heap directly, so the untracking
 * has to happen in the allocator rather than in any k_* wrapper.
 */
ZTEST(obj_tracking_free, test_tracked_object_removed_on_multi_heap_free)
{
	static struct sys_multi_heap mheap;
	static char __aligned(8) arena[1024];
	struct k_sem *heap_sem;
	uintptr_t freed_addr;

	sys_heap_init(&heap, arena, sizeof(arena));
	sys_multi_heap_init(&mheap, multi_heap_choice);
	sys_multi_heap_add_heap(&mheap, &heap, NULL);

	heap_sem = sys_multi_heap_alloc(&mheap, NULL, sizeof(struct k_sem));
	zassert_not_null(heap_sem, "Failed to allocate semaphore");

	k_sem_init(heap_sem, 0, 1);
	freed_addr = (uintptr_t)heap_sem;
	zassert_equal(count_sem(freed_addr), 1, "Heap semaphore was not tracked");

	sys_multi_heap_free(&mheap, heap_sem);
	zassert_equal(count_sem(freed_addr), 0, "Semaphore is still tracked after multi-heap free");
}

/* An object placed exactly at the split boundary is the case the shrink hook
 * can miss: the first caller bytes released by the split sit below
 * chunk_mem() of the suffix chunk, where the new chunk header lands.
 */
ZTEST(obj_tracking_free, test_tracked_object_at_shrink_boundary_removed)
{
	struct k_sem *sems;
	struct k_sem *boundary_sem;
	uintptr_t boundary_addr;
	void *shrunk;

	sems = k_malloc(4 * sizeof(struct k_sem));
	zassert_not_null(sems, "Failed to allocate the block");

	/* Keep sems[0..1], release from sems[2] onward */
	boundary_sem = &sems[2];
	k_sem_init(boundary_sem, 0, 1);
	boundary_addr = (uintptr_t)boundary_sem;
	zassert_equal(count_sem(boundary_addr), 1, "Boundary semaphore was not tracked");

	shrunk = k_realloc(sems, 2 * sizeof(struct k_sem));
	zassert_not_null(shrunk, "Failed to shrink the allocation");

	if ((uintptr_t)shrunk != (uintptr_t)sems) {
		k_free(shrunk);
		ztest_test_skip();
	}

	zassert_equal(count_sem(boundary_addr), 0,
		      "Semaphore at the split boundary is still tracked");

	k_free(shrunk);
}

/* An in-place shrink can cut through a tracked object: its start stays in the
 * retained region while its tail, including the tracking link, goes back to
 * the heap. Overlap matching has to drop it; matching on the start address
 * alone would keep a node whose link field lives in reused memory.
 */
ZTEST(obj_tracking_free, test_tracked_object_straddling_shrink_cut_removed)
{
	struct k_sem *sems;
	uintptr_t straddler_addr;
	void *shrunk;

	sems = k_malloc(4 * sizeof(struct k_sem));
	zassert_not_null(sems, "Failed to allocate the block");

	/* sems[1] spans the cut: the shrink keeps one and a half semaphores,
	 * so the released region starts inside it.
	 */
	k_sem_init(&sems[1], 0, 1);
	straddler_addr = (uintptr_t)&sems[1];
	zassert_equal(count_sem(straddler_addr), 1, "Straddling semaphore was not tracked");

	shrunk = k_realloc(sems, sizeof(struct k_sem) + sizeof(struct k_sem) / 2);
	zassert_not_null(shrunk, "Failed to shrink the allocation");

	if ((uintptr_t)shrunk != (uintptr_t)sems) {
		k_free(shrunk);
		ztest_test_skip();
	}

	zassert_equal(count_sem(straddler_addr), 0,
		      "Semaphore straddling the shrink cut is still tracked");

	k_free(shrunk);
}

/* Counters for the remaining tracked types. Each list is walked with its own
 * element type, so the link offset is whatever that struct declares.
 */
static int count_mutex(uintptr_t target)
{
	void *list = _track_list_k_mutex;
	int count = 0;

	while (list != NULL) {
		if ((uintptr_t)list == target) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_mutex *)list);
	}

	return count;
}

static int count_queue(uintptr_t target)
{
	void *list = _track_list_k_queue;
	int count = 0;

	while (list != NULL) {
		if ((uintptr_t)list == target) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_queue *)list);
	}

	return count;
}

static int count_mbox(uintptr_t target)
{
	void *list = _track_list_k_mbox;
	int count = 0;

	while (list != NULL) {
		if ((uintptr_t)list == target) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_mbox *)list);
	}

	return count;
}

static int count_mem_slab(uintptr_t target)
{
	void *list = _track_list_k_mem_slab;
	int count = 0;

	while (list != NULL) {
		if ((uintptr_t)list == target) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_mem_slab *)list);
	}

	return count;
}

#ifdef CONFIG_EVENTS
static int count_event(uintptr_t target)
{
	void *list = _track_list_k_event;
	int count = 0;

	while (list != NULL) {
		if ((uintptr_t)list == target) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_event *)list);
	}

	return count;
}
#endif /* CONFIG_EVENTS */

/* sys_track_free() scans every tracking list, not just the semaphore one. Each
 * remaining type gets the same free-then-check treatment so a list that is
 * skipped or walked with the wrong link offset cannot pass unnoticed.
 */
ZTEST(obj_tracking_free, test_all_tracked_types_removed_on_free)
{
	struct k_mutex *mutex;
	struct k_queue *queue;
	struct k_mbox *mbox;
	struct k_mem_slab *slab;
	static char __aligned(8) slab_buffer[64];
	uintptr_t addr;

	mutex = k_malloc(sizeof(struct k_mutex));
	zassert_not_null(mutex, "Failed to allocate mutex");
	zassert_ok(k_mutex_init(mutex), "Failed to init mutex");
	addr = (uintptr_t)mutex;
	zassert_equal(count_mutex(addr), 1, "Mutex was not tracked");
	k_free(mutex);
	zassert_equal(count_mutex(addr), 0, "Freed mutex is still tracked");

	queue = k_malloc(sizeof(struct k_queue));
	zassert_not_null(queue, "Failed to allocate queue");
	k_queue_init(queue);
	addr = (uintptr_t)queue;
	zassert_equal(count_queue(addr), 1, "Queue was not tracked");
	k_free(queue);
	zassert_equal(count_queue(addr), 0, "Freed queue is still tracked");

	mbox = k_malloc(sizeof(struct k_mbox));
	zassert_not_null(mbox, "Failed to allocate mailbox");
	k_mbox_init(mbox);
	addr = (uintptr_t)mbox;
	zassert_equal(count_mbox(addr), 1, "Mailbox was not tracked");
	k_free(mbox);
	zassert_equal(count_mbox(addr), 0, "Freed mailbox is still tracked");

	slab = k_malloc(sizeof(struct k_mem_slab));
	zassert_not_null(slab, "Failed to allocate memory slab");
	zassert_ok(k_mem_slab_init(slab, slab_buffer, 8, 8), "Failed to init slab");
	addr = (uintptr_t)slab;
	zassert_equal(count_mem_slab(addr), 1, "Memory slab was not tracked");
	k_free(slab);
	zassert_equal(count_mem_slab(addr), 0, "Freed memory slab is still tracked");
}

#ifdef CONFIG_EVENTS
/* The event list is the one type guarded by CONFIG_EVENTS, so it is scanned
 * from a separate branch of sys_track_free() and needs its own coverage.
 */
ZTEST(obj_tracking_free, test_tracked_event_removed_on_free)
{
	struct k_event *event;
	uintptr_t addr;

	event = k_malloc(sizeof(struct k_event));
	zassert_not_null(event, "Failed to allocate event");

	k_event_init(event);
	addr = (uintptr_t)event;
	zassert_equal(count_event(addr), 1, "Event was not tracked");

	k_free(event);
	zassert_equal(count_event(addr), 0, "Freed event is still tracked");
}
#endif /* CONFIG_EVENTS */

/* An aligned allocation hands back a pointer that can sit past the start of
 * the chunk, so the released range is measured from the chunk rather than from
 * the returned pointer. Freeing such a block still has to drop the object.
 */
ZTEST(obj_tracking_free, test_tracked_object_removed_on_aligned_free)
{
	static const size_t aligns[] = {8, 16, 32, 64};

	ARRAY_FOR_EACH(aligns, i) {
		struct k_sem *sem;
		uintptr_t addr;

		sem = k_aligned_alloc(aligns[i], sizeof(struct k_sem));
		zassert_not_null(sem, "Failed to allocate aligned semaphore");
		zassert_equal((uintptr_t)sem & (aligns[i] - 1), 0,
			      "Allocation is not aligned to %zu", aligns[i]);

		k_sem_init(sem, 0, 1);
		addr = (uintptr_t)sem;
		zassert_equal(count_sem(addr), 1, "Aligned semaphore was not tracked (align %zu)",
			      aligns[i]);

		k_free(sem);
		zassert_equal(count_sem(addr), 0,
			      "Freed aligned semaphore is still tracked (align %zu)", aligns[i]);
	}
}

/* A single freed block can hold more than one tracked object. Every node in
 * the range has to go, including the first and the last element, which are the
 * ones an off-by-one in the range check would miss.
 */
ZTEST(obj_tracking_free, test_multiple_objects_in_one_block_removed)
{
	const int count = 8;
	struct k_sem *sems;
	uintptr_t addrs[8];

	sems = k_malloc(count * sizeof(struct k_sem));
	zassert_not_null(sems, "Failed to allocate the block");

	for (int i = 0; i < count; i++) {
		k_sem_init(&sems[i], 0, 1);
		addrs[i] = (uintptr_t)&sems[i];
		zassert_equal(count_sem(addrs[i]), 1, "Semaphore %d was not tracked", i);
	}

	k_free(sems);

	for (int i = 0; i < count; i++) {
		zassert_equal(count_sem(addrs[i]), 0, "Semaphore %d is still tracked after free",
			      i);
	}
}

/* An object that neighbours a freed block must survive it. This is what an
 * over-wide range check would break, and neither a static object nor a
 * separately allocated one may be dropped.
 */
ZTEST(obj_tracking_free, test_neighbouring_objects_survive_free)
{
	static struct k_sem static_sem;
	struct k_sem *first;
	struct k_sem *second;
	struct k_sem *third;
	uintptr_t freed_addr;

	k_sem_init(&static_sem, 0, 1);

	first = k_malloc(sizeof(struct k_sem));
	second = k_malloc(sizeof(struct k_sem));
	third = k_malloc(sizeof(struct k_sem));
	zassert_not_null(first, "Failed to allocate first semaphore");
	zassert_not_null(second, "Failed to allocate second semaphore");
	zassert_not_null(third, "Failed to allocate third semaphore");

	k_sem_init(first, 0, 1);
	k_sem_init(second, 0, 1);
	k_sem_init(third, 0, 1);

	freed_addr = (uintptr_t)second;
	k_free(second);

	zassert_equal(count_sem(freed_addr), 0, "Freed semaphore is still tracked");
	zassert_equal(count_sem((uintptr_t)first), 1, "Lower neighbour was dropped by the free");
	zassert_equal(count_sem((uintptr_t)third), 1, "Upper neighbour was dropped by the free");
	zassert_equal(count_sem((uintptr_t)&static_sem), 1,
		      "Static semaphore was dropped by a heap free");

	k_free(first);
	k_free(third);
}

/* Removing the head, the middle and the tail of a list all take different
 * branches of the unlink loop, and the list has to stay walkable after each.
 */
ZTEST(obj_tracking_free, test_removal_from_any_list_position)
{
	struct k_sem *sems[3];
	uintptr_t addrs[3];

	ARRAY_FOR_EACH(sems, i) {
		sems[i] = k_malloc(sizeof(struct k_sem));
		zassert_not_null(sems[i], "Failed to allocate semaphore %zu", i);
		k_sem_init(sems[i], 0, 1);
		addrs[i] = (uintptr_t)sems[i];
	}

	/* Prepend order means sems[2] is the head and sems[0] the tail. Free
	 * the middle first, then the head, then what is left.
	 */
	k_free(sems[1]);
	zassert_equal(count_sem(addrs[1]), 0, "Middle entry is still tracked");
	zassert_equal(count_sem(addrs[0]), 1, "Tail entry was lost");
	zassert_equal(count_sem(addrs[2]), 1, "Head entry was lost");

	k_free(sems[2]);
	zassert_equal(count_sem(addrs[2]), 0, "Head entry is still tracked");
	zassert_equal(count_sem(addrs[0]), 1, "Tail entry was lost");

	k_free(sems[0]);
	zassert_equal(count_sem(addrs[0]), 0, "Tail entry is still tracked");
}

/* Re-initializing an object at an address that was freed must track it once,
 * not twice: the stale node has to be gone rather than merely unreachable.
 */
ZTEST(obj_tracking_free, test_reinit_after_free_tracks_once)
{
	struct k_sem *sem;
	uintptr_t addr;

	sem = k_malloc(sizeof(struct k_sem));
	zassert_not_null(sem, "Failed to allocate semaphore");
	k_sem_init(sem, 0, 1);
	addr = (uintptr_t)sem;
	zassert_equal(count_sem(addr), 1, "Semaphore was not tracked");

	k_free(sem);

	/* Same size request, so the allocator normally hands the block
	 * straight back. Reuse is allocator behavior, not part of the
	 * contract under test, so a different address skips instead of
	 * failing.
	 */
	sem = k_malloc(sizeof(struct k_sem));
	zassert_not_null(sem, "Failed to reallocate semaphore");
	if ((uintptr_t)sem != addr) {
		k_free(sem);
		ztest_test_skip();
	}

	k_sem_init(sem, 0, 1);
	zassert_equal(count_sem(addr), 1, "Re-initialized semaphore is tracked twice");

	k_free(sem);
	zassert_equal(count_sem(addr), 0, "Semaphore is still tracked");
}

/* Freeing a block that holds no tracked object at all must leave every list
 * untouched, and a zero-sized or NULL free must be harmless.
 */
ZTEST(obj_tracking_free, test_untracked_free_leaves_lists_intact)
{
	static struct k_sem sentinel;
	void *plain;

	k_sem_init(&sentinel, 0, 1);
	zassert_equal(count_sem((uintptr_t)&sentinel), 1, "Sentinel was not tracked");

	plain = k_malloc(64);
	zassert_not_null(plain, "Failed to allocate a plain block");
	memset(plain, 0x5A, 64);
	k_free(plain);

	k_free(NULL);

	zassert_equal(count_sem((uintptr_t)&sentinel), 1,
		      "An untracked free disturbed the semaphore list");
}

ZTEST_SUITE(obj_tracking_free, NULL, NULL, NULL, NULL, NULL);

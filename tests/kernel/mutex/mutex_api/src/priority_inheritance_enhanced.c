/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Enhanced priority inheritance tests for mutex priority inheritance
 *
 * This file contains tests that validate the new priority inheritance
 * enhancements, specifically:
 *
 *  - Chain priority boost across multiple threads (held_mutexes list walk)
 *  - Correct priority restoration when holding multiple mutexes
 *  - mutex_pended_on field lifecycle (set on contention, cleared on wakeup)
 *  - held_mutexes list integrity (add on lock, remove on unlock, handoff)
 *  - Recursive lock does not duplicate entry in held_mutexes
 *  - Timeout path: priority-down scans all held mutexes
 *  - K_NO_WAIT: no boost, no list modification
 *  - No false deadlock when a non-closing hop has a finite timeout
 *  - Chain walk hop cap truncates a long equal-priority chain
 *  - held_mutexes scan picks the true max across 2+ non-empty entries
 *  - Handoff to a new owner that already holds another mutex
 *  - Chain boost correctness through 3 hops / 4 threads
 *
 * Deadlock cycle detection is covered separately in
 * tests/kernel/mutex/mutex_deadlock/.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/* Forward declarations */
static void t_waiter(void *p1, void *p2, void *p3);
static void t_timeout_waiter(void *p1, void *p2, void *p3);
static void t_handoff_new_owner(void *p1, void *p2, void *p3);

#define STACK_SIZE  (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define TIMEOUT_MS  200

/* Priority levels */
#define PRIO_HIGH    1
#define PRIO_MID     3
#define PRIO_LOW     5
#define PRIO_ORIG    7   /* original priority of the owner thread */

/*
 * Mirrors kernel/mutex.c's private MUTEX_CHAIN_WALK_MAX_HOPS. Kept in sync
 * manually since the constant is not exposed via any header.
 */
#define TEST_CHAIN_WALK_MAX_HOPS 16
#define NUM_HOP_THREADS (TEST_CHAIN_WALK_MAX_HOPS + 2)

/* Shared mutexes */
static K_MUTEX_DEFINE(mutex_a);
static K_MUTEX_DEFINE(mutex_b);
static K_MUTEX_DEFINE(mutex_c);

/* Semaphores for thread synchronization */
static K_SEM_DEFINE(sem_low_ready,  0, 1);
static K_SEM_DEFINE(sem_med_ready,  0, 1);
static K_SEM_DEFINE(sem_low_go,     0, 1);
static K_SEM_DEFINE(sem_done,       0, NUM_HOP_THREADS + 1);
static K_SEM_DEFINE(sem_ready,      0, NUM_HOP_THREADS + 1);

/* Thread objects */
static K_THREAD_STACK_DEFINE(stack_low,  STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_med,  STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_high, STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_extra, STACK_SIZE);
static K_THREAD_STACK_ARRAY_DEFINE(stack_hops, NUM_HOP_THREADS, STACK_SIZE);
static struct k_mutex mutex_hops[NUM_HOP_THREADS];

static struct k_thread t_low, t_med, t_high, t_extra;
static struct k_thread t_hops[NUM_HOP_THREADS];

static void t_low_chain(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&mutex_a, K_FOREVER);
	k_sem_give(&sem_low_ready);
	k_sem_take(&sem_low_go, K_FOREVER);
	k_mutex_unlock(&mutex_a);
	k_sem_give(&sem_done);
}

static void t_med_chain(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&mutex_b, K_FOREVER);
	k_sem_give(&sem_med_ready);
	k_mutex_lock(&mutex_a, K_FOREVER);
	k_mutex_unlock(&mutex_a);
	k_mutex_unlock(&mutex_b);
	k_sem_give(&sem_done);
}

/**
 * @brief Verify chain priority boost propagates through 3 threads
 *
 * T_low holds mutex_a, T_med holds mutex_b and pends on mutex_a,
 * T_high pends on mutex_b. When T_high pends, the chain walk must boost
 * T_low all the way to T_high's priority, not just T_med.
 *
 * This validates the held_mutexes chain walk in k_mutex_lock().
 */
ZTEST(mutex_api_1cpu, test_chain_boost_3threads)
{
	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_low_ready);
	k_sem_reset(&sem_med_ready);
	k_sem_reset(&sem_low_go);
	k_sem_reset(&sem_done);

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_low_chain, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_LOW), 0, K_NO_WAIT);

	k_sem_take(&sem_low_ready, K_FOREVER);

	k_thread_create(&t_med, stack_med, STACK_SIZE,
			t_med_chain, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);

	k_sem_take(&sem_med_ready, K_FOREVER);

	/* T_med is now pending on mutex_a; T_low has been boosted to PRIO_MID */

	k_thread_create(&t_high, stack_high, STACK_SIZE,
			t_waiter, &mutex_b, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);

	k_sleep(K_MSEC(10));

	zassert_equal(t_low.base.prio, PRIO_HIGH,
		      "T_low not boosted to PRIO_HIGH via chain (got %d)",
		      t_low.base.prio);
	zassert_equal(t_med.base.prio, PRIO_HIGH,
		      "T_med not boosted to PRIO_HIGH (got %d)",
		      t_med.base.prio);

	k_sem_give(&sem_low_go);

	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);

	k_thread_join(&t_low,  K_FOREVER);
	k_thread_join(&t_med,  K_FOREVER);
	k_thread_join(&t_high, K_FOREVER);
}

static void t_waiter(void *p1, void *p2, void *p3)
{
	struct k_mutex *m = (struct k_mutex *)p1;

	k_mutex_lock(m, K_FOREVER);
	k_mutex_unlock(m);
	k_sem_give(&sem_done);
}

/**
 * @brief Verify priority drops correctly when unlocking one of multiple held mutexes
 *
 * T_owner holds mutex_a and mutex_b. T_high1 waits on mutex_a, T_high2 waits
 * on mutex_b. After unlocking mutex_a, T_owner's priority must drop to
 * T_high2's priority (not to original), because mutex_b still has a waiter.
 * After unlocking mutex_b, priority is fully restored.
 *
 * This validates held_mutexes_highest_waiter_prio() in k_mutex_unlock().
 */
ZTEST(mutex_api_1cpu, test_multi_mutex_partial_unlock_priority)
{
	int orig_prio;

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_done);

	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	k_mutex_lock(&mutex_a, K_FOREVER);
	k_mutex_lock(&mutex_b, K_FOREVER);

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_waiter, &mutex_a, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);

	k_thread_create(&t_med, stack_med, STACK_SIZE,
			t_waiter, &mutex_b, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);

	k_sleep(K_MSEC(10));

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_HIGH,
		      "Expected boost to PRIO_HIGH, got %d",
		      k_thread_priority_get(k_current_get()));

	k_mutex_unlock(&mutex_a);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_MID,
		      "After unlocking mutex_a, expected prio PRIO_MID, got %d",
		      k_thread_priority_get(k_current_get()));

	k_mutex_unlock(&mutex_b);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_ORIG,
		      "After unlocking all mutexes, expected prio PRIO_ORIG, got %d",
		      k_thread_priority_get(k_current_get()));

	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);

	k_thread_join(&t_low, K_FOREVER);
	k_thread_join(&t_med, K_FOREVER);

	k_thread_priority_set(k_current_get(), orig_prio);
}

/**
 * @brief Verify mutex_pended_on is NULL after lock is granted
 *
 * When a thread pends on a contended mutex and is eventually granted
 * ownership, mutex_pended_on must be cleared before k_mutex_lock() returns.
 * A stale pointer here would cause false deadlock detection if the thread
 * later tries to lock another mutex.
 */
ZTEST(mutex_api_1cpu, test_mutex_pended_on_cleared_on_grant)
{
#if Z_MUTEX_PI_ENABLED
	int orig_prio;

	k_mutex_init(&mutex_a);
	k_sem_reset(&sem_done);

	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	k_mutex_lock(&mutex_a, K_FOREVER);

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_waiter, &mutex_a, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);

	k_sleep(K_MSEC(10));

	k_mutex_unlock(&mutex_a);

	k_sleep(K_MSEC(10));

	zassert_is_null(t_low.mutex_pended_on,
			"mutex_pended_on not cleared after lock granted");

	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_low, K_FOREVER);

	k_thread_priority_set(k_current_get(), orig_prio);
#else
	ztest_test_skip();
#endif
}

static void t_timeout_waiter(void *p1, void *p2, void *p3)
{
	struct k_mutex *m = (struct k_mutex *)p1;
	k_timeout_t timeout = *(k_timeout_t *)p2;

	k_mutex_lock(m, timeout);
	k_sem_give(&sem_done);
}

/**
 * @brief Verify mutex_pended_on is NULL after timeout
 *
 * When a thread times out waiting for a mutex, mutex_pended_on must be
 * cleared before k_mutex_lock() returns -EAGAIN. A stale pointer here
 * could cause false deadlock detection in subsequent lock attempts by
 * other threads walking the chain.
 */
ZTEST(mutex_api_1cpu, test_mutex_pended_on_cleared_on_timeout)
{
#if Z_MUTEX_PI_ENABLED
	static k_timeout_t timeout = K_MSEC(TIMEOUT_MS);
	int orig_prio;

	k_mutex_init(&mutex_a);
	k_sem_reset(&sem_done);

	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	k_mutex_lock(&mutex_a, K_FOREVER);

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_timeout_waiter, &mutex_a, &timeout, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);

	k_sem_take(&sem_done, K_FOREVER);

	zassert_is_null(t_low.mutex_pended_on,
			"mutex_pended_on not cleared after timeout");

	k_mutex_unlock(&mutex_a);
	k_thread_join(&t_low, K_FOREVER);

	k_thread_priority_set(k_current_get(), orig_prio);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify held_mutexes list integrity and ownership handoff
 *
 * The held_mutexes list must accurately reflect which mutexes a thread
 * currently owns. Verifies that the mutex is added on first lock, transferred
 * to the new owner's list on unlock handoff, and removed after final unlock.
 */
ZTEST(mutex_api_1cpu, test_held_mutexes_list_and_handoff)
{
#if Z_MUTEX_PI_ENABLED
	int orig_prio;

	k_mutex_init(&mutex_a);
	k_sem_reset(&sem_done);

	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	k_mutex_lock(&mutex_a, K_FOREVER);
	zassert_false(sys_slist_is_empty(&k_current_get()->held_mutexes),
		      "held_mutexes empty after locking mutex_a");

	/*
	 * Use a lower-priority waiter so main is not preempted on unlock,
	 * allowing the held_mutexes check before the waiter runs.
	 */
	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_waiter, &mutex_a, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_ORIG + 1), 0, K_NO_WAIT);

	k_sleep(K_MSEC(10));

	k_mutex_unlock(&mutex_a);

	zassert_true(sys_slist_is_empty(&k_current_get()->held_mutexes),
		     "held_mutexes not empty after unlocking mutex_a");

	zassert_false(sys_slist_is_empty(&t_low.held_mutexes),
		      "new owner's held_mutexes empty after handoff");

	k_sem_take(&sem_done, K_FOREVER);

	zassert_true(sys_slist_is_empty(&t_low.held_mutexes),
		     "new owner's held_mutexes not empty after final unlock");

	k_thread_join(&t_low, K_FOREVER);

	k_thread_priority_set(k_current_get(), orig_prio);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify recursive lock does not add mutex twice to held_mutexes
 *
 * A recursive lock increments lock_count but must not add the mutex to
 * held_mutexes again. The mutex must appear exactly once in the list
 * regardless of recursion depth, and be removed on the final unlock.
 */
ZTEST(mutex_api_1cpu, test_recursive_held_mutexes_single_entry)
{
#if Z_MUTEX_PI_ENABLED
	sys_snode_t *node;
	int count;

	k_mutex_init(&mutex_a);

	k_mutex_lock(&mutex_a, K_FOREVER);
	count = 0;
	SYS_SLIST_FOR_EACH_NODE(&k_current_get()->held_mutexes, node) {
		if (CONTAINER_OF(node, struct k_mutex, held_node) == &mutex_a) {
			count++;
		}
	}
	zassert_equal(count, 1,
		      "mutex_a should appear once after first lock, got %d", count);

	k_mutex_lock(&mutex_a, K_FOREVER);
	count = 0;
	SYS_SLIST_FOR_EACH_NODE(&k_current_get()->held_mutexes, node) {
		if (CONTAINER_OF(node, struct k_mutex, held_node) == &mutex_a) {
			count++;
		}
	}
	zassert_equal(count, 1,
		      "mutex_a should appear once after recursive lock, got %d", count);

	k_mutex_unlock(&mutex_a);
	count = 0;
	SYS_SLIST_FOR_EACH_NODE(&k_current_get()->held_mutexes, node) {
		if (CONTAINER_OF(node, struct k_mutex, held_node) == &mutex_a) {
			count++;
		}
	}
	zassert_equal(count, 1,
		      "mutex_a should still be in list after first unlock, got %d", count);

	k_mutex_unlock(&mutex_a);
	count = 0;
	SYS_SLIST_FOR_EACH_NODE(&k_current_get()->held_mutexes, node) {
		if (CONTAINER_OF(node, struct k_mutex, held_node) == &mutex_a) {
			count++;
		}
	}
	zassert_equal(count, 0,
		      "mutex_a should be removed after final unlock, got %d", count);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify timeout priority-down scans all held mutexes
 *
 * T_owner holds mutex_a and mutex_b. T_high times out waiting on mutex_a,
 * T_mid waits on mutex_b. After T_high times out, T_owner's priority must
 * drop to T_mid's priority (not to original), because mutex_b still has a
 * waiter. This validates held_mutexes_highest_waiter_prio() in the timeout path.
 */
ZTEST(mutex_api_1cpu, test_timeout_multi_mutex_priority_down)
{
	static k_timeout_t timeout = K_MSEC(TIMEOUT_MS);
	int orig_prio;

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_done);

	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	k_mutex_lock(&mutex_a, K_FOREVER);
	k_mutex_lock(&mutex_b, K_FOREVER);

	k_thread_create(&t_high, stack_high, STACK_SIZE,
			t_timeout_waiter, &mutex_a, &timeout, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);

	k_thread_create(&t_med, stack_med, STACK_SIZE,
			t_waiter, &mutex_b, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);

	k_sem_take(&sem_done, K_FOREVER);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_MID,
		      "After T_high timeout, expected prio PRIO_MID, got %d",
		      k_thread_priority_get(k_current_get()));

	k_mutex_unlock(&mutex_b);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_ORIG,
		      "After releasing mutex_b, expected prio PRIO_ORIG, got %d",
		      k_thread_priority_get(k_current_get()));

	k_mutex_unlock(&mutex_a);

	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_high, K_FOREVER);
	k_thread_join(&t_med,  K_FOREVER);

	k_thread_priority_set(k_current_get(), orig_prio);
}

static void t_hold_mutex(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&mutex_a, K_FOREVER);
	k_sem_give(&sem_low_ready);
	k_sem_take(&sem_low_go, K_FOREVER);
	k_mutex_unlock(&mutex_a);
	k_sem_give(&sem_done);
}

static void t_hold_mutex_b_timeout(void *p1, void *p2, void *p3)
{
	k_timeout_t *to = (k_timeout_t *)p1;
	int ret;

	k_mutex_lock(&mutex_b, K_FOREVER);
	k_sem_give(&sem_low_ready);
	ret = k_mutex_lock(&mutex_a, *to);
	if (ret == 0) {
		k_mutex_unlock(&mutex_a);
	}
	k_mutex_unlock(&mutex_b);
	k_sem_give(&sem_done);
}

/**
 * @brief Verify orig_prio floor is correct when thread is already boosted
 *        at the time it acquires a second mutex
 *
 * T (prio 7) locks m1. W1 (prio 3) pends on m1, boosting T to 3.
 * T then locks m2 while already boosted. W2 (prio 1) pends on m2,
 * boosting T to 1. T unlocks m1 first (non-LIFO order). T must drop
 * to 1 (not 3), because m2 still has W2. After unlocking m2, T must
 * drop to 7 (not 3), proving orig_prio captured the true pre-inheritance
 * priority rather than the boosted snapshot.
 */
ZTEST(mutex_api_1cpu, test_orig_prio_floor_when_boosted_at_second_lock)
{
	int saved_prio;

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_done);

	saved_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	/* T locks m1 */
	k_mutex_lock(&mutex_a, K_FOREVER);

	/* W1 (prio 3) pends on m1 — boosts T to 3 */
	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_waiter, &mutex_a, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_MID,
		      "T should be boosted to PRIO_MID by W1, got %d",
		      k_thread_priority_get(k_current_get()));

	/* T locks m2 while already boosted to PRIO_MID */
	k_mutex_lock(&mutex_b, K_FOREVER);

	/* W2 (prio 1) pends on m2 — boosts T to 1 */
	k_thread_create(&t_med, stack_med, STACK_SIZE,
			t_waiter, &mutex_b, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_HIGH,
		      "T should be boosted to PRIO_HIGH by W2, got %d",
		      k_thread_priority_get(k_current_get()));

	/* T unlocks m1 first (non-LIFO) — must drop to PRIO_HIGH, not PRIO_ORIG */
	k_mutex_unlock(&mutex_a);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_HIGH,
		      "After unlocking m1, T should stay at PRIO_HIGH (m2 still has W2), got %d",
		      k_thread_priority_get(k_current_get()));

	/* T unlocks m2 — must drop to PRIO_ORIG (7), not PRIO_MID (3) */
	k_mutex_unlock(&mutex_b);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_ORIG,
		      "After unlocking m2, T should drop to PRIO_ORIG, got %d",
		      k_thread_priority_get(k_current_get()));

	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);

	k_thread_join(&t_low, K_FOREVER);
	k_thread_join(&t_med, K_FOREVER);

	k_thread_priority_set(k_current_get(), saved_prio);
}

/**
 * @brief Verify no false-positive deadlock when a cycle member has a finite timeout
 *
 * A holds m1 and waits on m2 with K_FOREVER. B holds m2 and waits on m1
 * with K_MSEC(100). This forms a cycle, but B's finite timeout means it
 * is NOT a true deadlock — B will recover via -EAGAIN. The deadlock
 * detection must NOT fire an assertion. After B times out and releases m2,
 * A must successfully acquire m2.
 */
ZTEST(mutex_api_1cpu, test_no_false_deadlock_finite_timeout_cycle)
{
	static k_timeout_t b_timeout = K_MSEC(200);
	int ret;
	int saved_prio;

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_low_ready);
	k_sem_reset(&sem_done);

	saved_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	/* A (main) locks m1 */
	k_mutex_lock(&mutex_a, K_FOREVER);

	/* B locks m2, then tries to lock m1 with finite timeout */
	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_hold_mutex_b_timeout, &b_timeout, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);

	/* Wait for B to hold m2 */
	k_sem_take(&sem_low_ready, K_FOREVER);
	k_sleep(K_MSEC(10));

	/*
	 * A now tries to lock m2 with K_FOREVER. This forms a cycle:
	 * A→m2→B→m1→A. But B has a finite timeout, so it is NOT a true
	 * deadlock. No assertion must fire.
	 */
	ret = k_mutex_lock(&mutex_b, K_FOREVER);
	zassert_equal(ret, 0,
		      "A should acquire m2 after B times out, got %d", ret);

	k_mutex_unlock(&mutex_b);
	k_mutex_unlock(&mutex_a);

	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_low, K_FOREVER);

	k_thread_priority_set(k_current_get(), saved_prio);
}

/**
 * @brief Verify K_NO_WAIT on contended mutex has no side effects
 *
 * When k_mutex_lock() is called with K_NO_WAIT and the mutex is held by
 * another thread, it must return -EBUSY immediately without boosting the
 * owner's priority, modifying held_mutexes, or setting mutex_pended_on.
 */
ZTEST(mutex_api_1cpu, test_no_wait_no_boost_no_list)
{
	int ret;
	int orig_prio;

	k_mutex_init(&mutex_a);
	k_sem_reset(&sem_low_ready);
	k_sem_reset(&sem_low_go);
	k_sem_reset(&sem_done);

	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_HIGH));

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_hold_mutex, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_LOW), 0, K_NO_WAIT);

	k_sem_take(&sem_low_ready, K_FOREVER);

	int owner_prio_before = k_thread_priority_get(&t_low);

	ret = k_mutex_lock(&mutex_a, K_NO_WAIT);

	zassert_equal(ret, -EBUSY,
		      "K_NO_WAIT on contended mutex should return -EBUSY, got %d", ret);

	zassert_equal(k_thread_priority_get(&t_low), owner_prio_before,
		      "Owner priority changed after K_NO_WAIT attempt");

#if Z_MUTEX_PI_ENABLED
	zassert_true(sys_slist_is_empty(&k_current_get()->held_mutexes),
		     "held_mutexes modified by K_NO_WAIT attempt");

	zassert_is_null(k_current_get()->mutex_pended_on,
			"mutex_pended_on set by K_NO_WAIT attempt");
#endif

	k_sem_give(&sem_low_go);
	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_low, K_FOREVER);

	k_thread_priority_set(k_current_get(), orig_prio);
}


/*
 * Generic two-mutex chain link: locks *hold_mutex, signals ready, then
 * blocks on *wait_mutex with the given timeout. Used to build chains of
 * arbitrary length/topology without one bespoke thread function per test.
 */
struct chain_link_args {
	struct k_mutex *hold_mutex;
	struct k_mutex *wait_mutex;
	k_timeout_t timeout;
};

static void t_chain_link(void *p1, void *p2, void *p3)
{
	struct chain_link_args *args = (struct chain_link_args *)p1;

	k_mutex_lock(args->hold_mutex, K_FOREVER);
	k_sem_give(&sem_ready);
	if (k_mutex_lock(args->wait_mutex, args->timeout) == 0) {
		k_mutex_unlock(args->wait_mutex);
	}
	k_mutex_unlock(args->hold_mutex);
	k_sem_give(&sem_done);
}

/**
 * @brief Verify no false deadlock when a non-closing hop has a finite timeout
 *
 * A (main) holds m1, tries to lock m2 with K_FOREVER. B holds m2, waits
 * K_FOREVER on m3. C holds m3, waits K_MSEC(...) on m1. This forms the
 * cycle A->m2->B->m3->C->m1->A. The hop that closes the cycle back to
 * _current is C (C waits on m1, owned by A = _current); C itself waits
 * only a finite time, so this must NOT be flagged as a deadlock even
 * though B -- an intermediate, non-closing hop -- waits forever. This
 * complements test_no_false_deadlock_finite_timeout_cycle (which only
 * covers the 2-hop case, where the finite timeout is always at the
 * closing hop) by putting the finite timeout at a different position.
 */
ZTEST(mutex_api_1cpu, test_no_false_deadlock_intermediate_hop_finite_timeout)
{
	static struct chain_link_args b_args, c_args;
	int ret;
	int saved_prio;

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_mutex_init(&mutex_c);
	k_sem_reset(&sem_ready);
	k_sem_reset(&sem_done);

	saved_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	/* A (main) locks m1 */
	k_mutex_lock(&mutex_a, K_FOREVER);

	/* B locks m2, then waits K_FOREVER on m3 */
	b_args.hold_mutex = &mutex_b;
	b_args.wait_mutex = &mutex_c;
	b_args.timeout = K_FOREVER;
	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_chain_link, &b_args, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);
	k_sem_take(&sem_ready, K_FOREVER);
	k_sleep(K_MSEC(10));

	/* C locks m3, then waits a finite timeout on m1 (owned by A) */
	c_args.hold_mutex = &mutex_c;
	c_args.wait_mutex = &mutex_a;
	c_args.timeout = K_MSEC(TIMEOUT_MS);
	k_thread_create(&t_extra, stack_extra, STACK_SIZE,
			t_chain_link, &c_args, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_LOW), 0, K_NO_WAIT);
	k_sem_take(&sem_ready, K_FOREVER);
	k_sleep(K_MSEC(10));

	/*
	 * A now tries to lock m2 with K_FOREVER, closing the cycle
	 * A->m2->B->m3->C->m1->A. C has a finite timeout, so no assert
	 * must fire. C eventually times out on m1, releases m3, and B's
	 * wait on m3 (and A's wait on m2) both resolve.
	 */
	ret = k_mutex_lock(&mutex_b, K_FOREVER);
	zassert_equal(ret, 0,
		      "A should acquire m2 once C times out, got %d", ret);

	k_mutex_unlock(&mutex_b);
	k_mutex_unlock(&mutex_a);

	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_low, K_FOREVER);
	k_thread_join(&t_extra, K_FOREVER);

	k_thread_priority_set(k_current_get(), saved_prio);
}

/**
 * @brief Verify the chain walk hop cap truncates a long equal-priority chain
 *
 * Builds a chain of NUM_HOP_THREADS (TEST_CHAIN_WALK_MAX_HOPS + 2) threads:
 * thread i holds mutex_hops[i] and blocks K_FOREVER on mutex_hops[i - 1]
 * (thread 0 just holds mutex_hops[0] and returns immediately). All threads
 * run at equal priority so the walk's hop cap -- not a priority
 * short-circuit -- is what stops propagation. A final highest-priority
 * waiter blocks on the last mutex in the chain; threads beyond the hop
 * cap from that point must NOT be boosted. This chain does not close back
 * on _current, so no deadlock assertion is expected either way.
 */
static void t_hop_anchor(void *p1, void *p2, void *p3)
{
	struct k_mutex *m = (struct k_mutex *)p1;

	k_mutex_lock(m, K_FOREVER);
	k_sem_give(&sem_ready);
	k_sem_take(&sem_low_go, K_FOREVER);
	k_mutex_unlock(m);
	k_sem_give(&sem_done);
}

ZTEST(mutex_api_1cpu, test_chain_walk_hop_cap_truncates)
{
	static struct chain_link_args hop_args[NUM_HOP_THREADS];
	int i;

	k_sem_reset(&sem_ready);
	k_sem_reset(&sem_low_go);
	k_sem_reset(&sem_done);

	for (i = 0; i < NUM_HOP_THREADS; i++) {
		k_mutex_init(&mutex_hops[i]);
	}

	/*
	 * Thread 0 holds mutex_hops[0] and simply parks on sem_low_go
	 * (it does not block on any mutex, so it is the fixed "anchor" at
	 * the deep end of the chain). Thread i (i > 0) holds mutex_hops[i]
	 * and blocks K_FOREVER on mutex_hops[i - 1]. Threads are created
	 * from index 0 upward so each one's wait_mutex is already owned
	 * by the time it tries to lock it.
	 */
	k_thread_create(&t_hops[0], stack_hops[0], STACK_SIZE,
			t_hop_anchor, &mutex_hops[0], NULL, NULL,
			K_PRIO_PREEMPT(PRIO_LOW), 0, K_NO_WAIT);
	k_sem_take(&sem_ready, K_FOREVER);

	for (i = 1; i < NUM_HOP_THREADS; i++) {
		hop_args[i].hold_mutex = &mutex_hops[i];
		hop_args[i].wait_mutex = &mutex_hops[i - 1];
		hop_args[i].timeout = K_FOREVER;

		k_thread_create(&t_hops[i], stack_hops[i], STACK_SIZE,
				t_chain_link, &hop_args[i], NULL, NULL,
				K_PRIO_PREEMPT(PRIO_LOW), 0, K_NO_WAIT);
		k_sem_take(&sem_ready, K_FOREVER);
		k_sleep(K_MSEC(5));
	}

	/*
	 * Highest-priority waiter blocks on the last mutex in the chain.
	 * The chain walk boosts owners starting there and working back
	 * towards index 0, capped at TEST_CHAIN_WALK_MAX_HOPS hops.
	 */
	k_thread_create(&t_high, stack_high, STACK_SIZE,
			t_waiter, &mutex_hops[NUM_HOP_THREADS - 1], NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);

	k_sleep(K_MSEC(10));

	/*
	 * Index NUM_HOP_THREADS - 1 is hop 0 (boosted unconditionally
	 * before the cap check); each step back towards index 0 consumes
	 * one more hop. Indices within TEST_CHAIN_WALK_MAX_HOPS of the
	 * start must be boosted; thread 0 (beyond the cap for this chain
	 * length: NUM_HOP_THREADS - 1 == TEST_CHAIN_WALK_MAX_HOPS + 1
	 * hops away) must not be.
	 */
	for (i = NUM_HOP_THREADS - 1;
	     i > NUM_HOP_THREADS - 1 - TEST_CHAIN_WALK_MAX_HOPS; i--) {
		zassert_equal(t_hops[i].base.prio, PRIO_HIGH,
			      "thread %d within hop cap not boosted (got %d)",
			      i, t_hops[i].base.prio);
	}

	zassert_not_equal(t_hops[0].base.prio, PRIO_HIGH,
			   "thread 0 beyond hop cap should not be boosted");

	/* Unwind from the anchor: releasing sem_low_go lets thread 0 unlock
	 * mutex_hops[0], which lets thread 1 acquire it and exit, and so on
	 * up the chain.
	 */
	k_sem_give(&sem_low_go);

	for (i = 0; i < NUM_HOP_THREADS; i++) {
		k_sem_take(&sem_done, K_FOREVER);
		k_thread_join(&t_hops[i], K_FOREVER);
	}
	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_high, K_FOREVER);
}

/**
 * @brief Verify held_mutexes scan picks the true max across 2+ live entries
 *
 * T_owner holds mutex_a and mutex_b simultaneously with two DIFFERENT
 * priority waiters pending at once (unlike test_multi_mutex_partial_unlock_
 * priority, which never has both waiters live at the same instant relative
 * to an unlock). Both waiters are added before either mutex is unlocked, so
 * held_mutexes_highest_waiter_prio() must scan both live entries and return
 * the correct maximum rather than whichever it happens to see first.
 */
ZTEST(mutex_api_1cpu, test_held_mutexes_highest_waiter_scans_all_entries)
{
	int orig_prio;

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_done);

	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_ORIG));

	k_mutex_lock(&mutex_a, K_FOREVER);
	k_mutex_lock(&mutex_b, K_FOREVER);

	/* Lower-priority waiter goes on mutex_a first. */
	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_waiter, &mutex_a, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	/*
	 * Higher-priority waiter goes on mutex_b second, while mutex_a's
	 * waiter is still pending -- both entries are now live at once.
	 */
	k_thread_create(&t_med, stack_med, STACK_SIZE,
			t_waiter, &mutex_b, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_HIGH,
		      "owner should be boosted to PRIO_HIGH (mutex_b's waiter), got %d",
		      k_thread_priority_get(k_current_get()));

	/*
	 * Unlock mutex_b (the higher-priority waiter's mutex) first. The
	 * scan of remaining held mutexes must still find mutex_a's
	 * PRIO_MID waiter, not fall through to orig_prio.
	 */
	k_mutex_unlock(&mutex_b);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_MID,
		      "owner should drop to PRIO_MID (mutex_a's waiter), got %d",
		      k_thread_priority_get(k_current_get()));

	k_mutex_unlock(&mutex_a);

	zassert_equal(k_thread_priority_get(k_current_get()), PRIO_ORIG,
		      "owner should fully restore to PRIO_ORIG, got %d",
		      k_thread_priority_get(k_current_get()));

	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_low, K_FOREVER);
	k_thread_join(&t_med, K_FOREVER);

	k_thread_priority_set(k_current_get(), orig_prio);
}

static void t_handoff_new_owner(void *p1, void *p2, void *p3)
{
	/* Already holds mutex_b when it becomes the new owner of mutex_a. */
	k_mutex_lock(&mutex_b, K_FOREVER);
	k_sem_give(&sem_low_ready);
	k_sem_take(&sem_low_go, K_FOREVER);
	k_mutex_lock(&mutex_a, K_FOREVER);
	/* Hold both mutexes until the test has inspected held_mutexes. */
	k_sem_give(&sem_ready);
	k_sem_take(&sem_low_go, K_FOREVER);
	k_mutex_unlock(&mutex_a);
	k_mutex_unlock(&mutex_b);
	k_sem_give(&sem_done);
}

/**
 * @brief Verify handoff to a new owner that already holds another mutex
 *
 * T_new already holds mutex_b (linked in its own held_mutexes) when it
 * becomes the new owner of mutex_a via unlock handoff. held_mutexes must
 * end up with BOTH mutex_a and mutex_b linked -- the handoff append must
 * not clobber or duplicate the list built by T_new's own prior lock.
 */
ZTEST(mutex_api_1cpu, test_handoff_to_owner_already_holding_another_mutex)
{
#if Z_MUTEX_PI_ENABLED
	sys_snode_t *node;
	int count_a, count_b;

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_low_ready);
	k_sem_reset(&sem_low_go);
	k_sem_reset(&sem_ready);
	k_sem_reset(&sem_done);

	k_mutex_lock(&mutex_a, K_FOREVER);

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_handoff_new_owner, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);

	/* T_new locks mutex_b, then blocks waiting on mutex_a. */
	k_sem_take(&sem_low_ready, K_FOREVER);
	k_sem_give(&sem_low_go);
	k_sleep(K_MSEC(10));

	/* Handoff: unlocking mutex_a transfers ownership to T_new. */
	k_mutex_unlock(&mutex_a);

	/* Wait until T_new has acquired mutex_a and is holding both. */
	k_sem_take(&sem_ready, K_FOREVER);

	count_a = 0;
	count_b = 0;
	SYS_SLIST_FOR_EACH_NODE(&t_low.held_mutexes, node) {
		struct k_mutex *m = CONTAINER_OF(node, struct k_mutex, held_node);

		if (m == &mutex_a) {
			count_a++;
		} else if (m == &mutex_b) {
			count_b++;
		}
	}

	zassert_equal(count_a, 1,
		      "mutex_a missing from new owner's held_mutexes after handoff");
	zassert_equal(count_b, 1,
		      "mutex_b (already held) lost from held_mutexes after handoff");

	/* Let T_new release both mutexes and exit. */
	k_sem_give(&sem_low_go);
	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_low, K_FOREVER);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify chain priority boost correctness through 3 hops / 4 threads
 *
 * T1 holds m1 (not pending on anything). T2 holds m2, pends K_FOREVER on
 * m1. T3 holds m3, pends K_FOREVER on m2. T4 (highest priority, created
 * last) pends on m3. The chain walk starting from T4's lock attempt must
 * boost T3, then continue past T3 to boost T2, then continue past T2 to
 * boost T1 -- a genuine 3-hop propagation through 3 distinct mutexes,
 * which test_chain_boost_3threads (2 mutexes, 2 hops) cannot exercise.
 */
ZTEST(mutex_api_1cpu, test_chain_boost_3hops_4threads)
{
	static struct chain_link_args t2_args, t3_args;

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_mutex_init(&mutex_c);
	k_sem_reset(&sem_ready);
	k_sem_reset(&sem_low_go);
	k_sem_reset(&sem_done);

	/* T1 (t_low) locks m1 and waits to be released. */
	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_hold_mutex, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_LOW), 0, K_NO_WAIT);
	k_sem_take(&sem_low_ready, K_FOREVER);

	/* T2 (t_med) locks m2, then blocks K_FOREVER on m1. */
	t2_args.hold_mutex = &mutex_b;
	t2_args.wait_mutex = &mutex_a;
	t2_args.timeout = K_FOREVER;
	k_thread_create(&t_med, stack_med, STACK_SIZE,
			t_chain_link, &t2_args, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);
	k_sem_take(&sem_ready, K_FOREVER);
	k_sleep(K_MSEC(10));

	/* T3 (t_extra) locks m3, then blocks K_FOREVER on m2. */
	t3_args.hold_mutex = &mutex_c;
	t3_args.wait_mutex = &mutex_b;
	t3_args.timeout = K_FOREVER;
	k_thread_create(&t_extra, stack_extra, STACK_SIZE,
			t_chain_link, &t3_args, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID + 1), 0, K_NO_WAIT);
	k_sem_take(&sem_ready, K_FOREVER);
	k_sleep(K_MSEC(10));

	/* T4 (t_high) blocks on m3, the deepest link in the chain. */
	k_thread_create(&t_high, stack_high, STACK_SIZE,
			t_waiter, &mutex_c, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	zassert_equal(t_extra.base.prio, PRIO_HIGH,
		      "T3 (hop 1) not boosted to PRIO_HIGH, got %d", t_extra.base.prio);
	zassert_equal(t_med.base.prio, PRIO_HIGH,
		      "T2 (hop 2) not boosted to PRIO_HIGH, got %d", t_med.base.prio);
	zassert_equal(t_low.base.prio, PRIO_HIGH,
		      "T1 (hop 3) not boosted to PRIO_HIGH, got %d", t_low.base.prio);

	k_sem_give(&sem_low_go);

	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);

	k_thread_join(&t_low, K_FOREVER);
	k_thread_join(&t_med, K_FOREVER);
	k_thread_join(&t_extra, K_FOREVER);
	k_thread_join(&t_high, K_FOREVER);
}

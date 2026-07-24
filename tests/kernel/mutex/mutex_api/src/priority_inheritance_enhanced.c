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
 *  - Deadlock detection (assert + block on deadlock cycle)
 *  - Correct priority restoration when holding multiple mutexes
 *  - mutex_pended_on field lifecycle (set on contention, cleared on wakeup)
 *  - held_mutexes list integrity (add on lock, remove on unlock, handoff)
 *  - Recursive lock does not duplicate entry in held_mutexes
 *  - Timeout path: priority-down scans all held mutexes
 *  - K_NO_WAIT: no boost, no list modification
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/kernel.h>

/* Forward declarations */
static void t_waiter(void *p1, void *p2, void *p3);
static void t_timeout_waiter(void *p1, void *p2, void *p3);

#define STACK_SIZE  (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define TIMEOUT_MS  200

/* Priority levels */
#define PRIO_HIGH    1
#define PRIO_MID     3
#define PRIO_LOW     5
#define PRIO_ORIG    7   /* original priority of the owner thread */

/* Shared mutexes */
static K_MUTEX_DEFINE(mutex_a);
static K_MUTEX_DEFINE(mutex_b);

/* Semaphores for thread synchronization */
static K_SEM_DEFINE(sem_low_ready,  0, 1);
static K_SEM_DEFINE(sem_med_ready,  0, 1);
static K_SEM_DEFINE(sem_low_go,     0, 1);
static K_SEM_DEFINE(sem_done,       0, 3);

/* Thread objects */
static K_THREAD_STACK_DEFINE(stack_low,  STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_med,  STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_high, STACK_SIZE);

static struct k_thread t_low, t_med, t_high;

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

static void t_b_deadlock(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&mutex_b, K_FOREVER);
	k_sem_give(&sem_low_ready);
	k_mutex_lock(&mutex_a, K_FOREVER);
	k_mutex_unlock(&mutex_a);
	k_mutex_unlock(&mutex_b);
	k_sem_give(&sem_done);
}

#if defined(CONFIG_MUTEX_DEADLOCK_DETECT)
/*
 * After test_deadlock_detection fires __ASSERT, the test function is aborted
 * before it can clean up. This hook runs after the fatal error is caught and
 * restores a clean state so subsequent tests are not affected by stale
 * threads or locked mutexes left behind by the aborted test.
 */
void ztest_post_fatal_error_hook(unsigned int reason,
				 const struct arch_esf *pEsf)
{
	/*
	 * Remove mutex_a from the main thread's held_mutexes list BEFORE
	 * reinitializing the mutex. k_mutex_init() sets held_node.next = NULL,
	 * which would corrupt the list if the node is still linked into it.
	 */
	sys_slist_find_and_remove(&k_current_get()->held_mutexes,
				  &mutex_a.held_node);

	/* Abort t_low which is stuck pending on mutex_a */
	k_thread_abort(&t_low);

	/* Reinitialize both mutexes to clear all stale state */
	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
}
#endif /* CONFIG_MUTEX_DEADLOCK_DETECT */

/**
 * @brief Verify deadlock cycle is detected and triggers an assertion
 *
 * Main holds mutex_a, T_b holds mutex_b and pends on mutex_a. When main
 * tries to lock mutex_b with K_FOREVER, the chain walk detects the cycle
 * (main→mutex_b→T_b→mutex_a→main) and fires __ASSERT.
 *
 * Requires CONFIG_MUTEX_DEADLOCK_DETECT and CONFIG_ASSERT.
 * ztest_set_fault_valid(true) tells the test framework to expect the
 * fatal error so the test passes rather than crashing.
 */
ZTEST(mutex_api_1cpu, test_deadlock_detection)
{
#if defined(CONFIG_MUTEX_DEADLOCK_DETECT)
	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_low_ready);
	k_sem_reset(&sem_done);

	k_mutex_lock(&mutex_a, K_FOREVER);

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_b_deadlock, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);

	k_sem_take(&sem_low_ready, K_FOREVER);

	/* Give T_b time to pend on mutex_a */
	k_sleep(K_MSEC(10));

	/*
	 * Deadlock: the chain walk detects the cycle and fires __ASSERT.
	 * Mark the expected fatal error so the test framework catches it.
	 */
	ztest_set_fault_valid(true);
	k_mutex_lock(&mutex_b, K_FOREVER);

	/* Should not be reached — the assert aborts the test function. */
	zassert_unreachable("deadlock should have triggered __ASSERT");
#else
	ztest_test_skip();
#endif /* CONFIG_MUTEX_DEADLOCK_DETECT */
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
#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
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
#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
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
#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
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
#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
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

#if (CONFIG_PRIORITY_CEILING < K_LOWEST_THREAD_PRIO)
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

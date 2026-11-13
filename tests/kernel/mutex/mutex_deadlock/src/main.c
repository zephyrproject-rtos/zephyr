/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Mutex deadlock cycle detection test
 *
 * This test intentionally triggers a fatal __ASSERT while mutex_lock
 * (kernel/mutex.c) is held, corrupting that spinlock's owner-tracking
 * state for the rest of the process. It must stay in a single-test
 * binary with no other mutex-touching suite — do not merge it into a
 * shared test app.
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/kernel.h>

#define STACK_SIZE  (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define PRIO_MID     3

static K_MUTEX_DEFINE(mutex_a);
static K_MUTEX_DEFINE(mutex_b);

static K_SEM_DEFINE(sem_low_ready, 0, 1);
static K_SEM_DEFINE(sem_unblock,   0, 1);
static K_SEM_DEFINE(sem_done,      0, 1);

#if defined(CONFIG_MUTEX_DEADLOCK_DETECT) && Z_MUTEX_PI_ENABLED
static K_THREAD_STACK_DEFINE(stack_low, STACK_SIZE);
static struct k_thread t_low;

static void unblock_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_sem_give(&sem_unblock);
}

static K_TIMER_DEFINE(unblock_timer, unblock_timer_handler, NULL);

static void t_b_deadlock(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&mutex_b, K_FOREVER);
	k_sem_give(&sem_low_ready);
	k_mutex_lock(&mutex_a, K_FOREVER);
	k_mutex_unlock(&mutex_a);
	k_mutex_unlock(&mutex_b);
	k_sem_give(&sem_done);
}

static void t_b_timeout_on_a(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(&mutex_b, K_FOREVER);
	k_sem_give(&sem_low_ready);
	(void)k_mutex_lock(&mutex_a, K_MSEC(5));
	k_mutex_unlock(&mutex_b);
	k_sem_give(&sem_done);
}

static void t_b_pend_on_sem_with_stale_mutex(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(&mutex_b, K_FOREVER);
	/*
	 * Simulate a stale mutex_pended_on pointer left over from a prior
	 * timed-out or aborted wait on mutex_a while this thread blocks with
	 * K_FOREVER on a non-mutex wait queue (sem_unblock).
	 */
	k_current_get()->mutex_pended_on = &mutex_a;
	k_sem_give(&sem_low_ready);
	k_sem_take(&sem_unblock, K_FOREVER);
	k_current_get()->mutex_pended_on = NULL;
	k_mutex_unlock(&mutex_b);
	k_sem_give(&sem_done);
}

/*
 * After test_deadlock_detection fires __ASSERT, the test function is
 * aborted before it can clean up. This hook runs after the fatal error
 * is caught and restores a clean state for subsequent tests.
 */
void ztest_post_fatal_error_hook(unsigned int reason,
				 const struct arch_esf *pEsf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);

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
#endif /* CONFIG_MUTEX_DEADLOCK_DETECT && Z_MUTEX_PI_ENABLED */

/**
 * @brief Verify deadlock cycle is detected and triggers an assertion
 *
 * Main holds mutex_a, T_b holds mutex_b and pends on mutex_a. When main
 * tries to lock mutex_b with K_FOREVER, the chain walk detects the cycle
 * (main→mutex_b→T_b→mutex_a→main) and fires __ASSERT.
 *
 * Requires CONFIG_MUTEX_DEADLOCK_DETECT, CONFIG_ASSERT, and priority
 * inheritance to be compiled in (Z_MUTEX_PI_ENABLED); the chain walk that
 * performs deadlock detection lives inside the PI code path.
 * ztest_set_fault_valid(true) tells the test framework to expect the
 * fatal error so the test passes rather than crashing.
 */
ZTEST(mutex_deadlock, test_deadlock_detection)
{
#if defined(CONFIG_MUTEX_DEADLOCK_DETECT) && Z_MUTEX_PI_ENABLED
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
#endif /* CONFIG_MUTEX_DEADLOCK_DETECT && Z_MUTEX_PI_ENABLED */
}

/**
 * @brief Verify mutex_lock spinlock is not left held after deadlock __ASSERT
 *
 * If __ASSERT fires inside z_impl_k_mutex_lock() while mutex_lock is still
 * held, any subsequent mutex operation (including printk/log hooks inside
 * assert_print() that attempt k_mutex_lock(..., K_NO_WAIT)) will fail
 * z_spin_lock_valid(&mutex_lock) and recursively fault/deadlock.
 */
ZTEST(mutex_deadlock, test_mutex_usable_after_deadlock_assert)
{
#if defined(CONFIG_MUTEX_DEADLOCK_DETECT) && Z_MUTEX_PI_ENABLED
	zassert_is_null(k_current_get()->mutex_pended_on,
			"mutex_pended_on was not cleared before deadlock __ASSERT");
	zassert_ok(k_mutex_lock(&mutex_a, K_NO_WAIT));
	zassert_ok(k_mutex_unlock(&mutex_a));
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify stale mutex_pended_on does not trigger a false-positive deadlock
 *
 * Exercises two mechanisms by which a stale mutex_pended_on pointer could
 * otherwise cause a false-positive deadlock assertion:
 *
 * 1. A thread (t_low at PRIO_MID) holds mutex_b and blocks on mutex_a (held
 *    by main at PRIO_MID) with a finite timeout. When t_low's timeout expires
 *    in ISR while main is running at equal priority, t_low is readied behind
 *    main with t_low.mutex_pended_on still pointing to mutex_a and an
 *    inactive timeout. When main then locks mutex_b with K_FOREVER while
 *    holding mutex_a, z_impl_k_mutex_lock() must see that t_low is no longer
 *    pending on mutex_a and must NOT trigger a false-positive deadlock assert.
 * 2. If a thread holding mutex_b has a stale mutex_pended_on pointing to
 *    mutex_a while pending with K_FOREVER on a non-mutex wait queue (a
 *    semaphore), locking mutex_b with K_FOREVER while holding mutex_a must
 *    verify chain_owner->base.pended_on == &chain_owner->mutex_pended_on->wait_q
 *    and NOT trigger a false-positive deadlock __ASSERT.
 */
ZTEST(mutex_deadlock, test_no_false_deadlock_on_stale_mutex_pended_on)
{
#if defined(CONFIG_MUTEX_DEADLOCK_DETECT) && Z_MUTEX_PI_ENABLED
	int orig_prio = k_thread_priority_get(k_current_get());

	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);
	k_sem_reset(&sem_low_ready);
	k_sem_reset(&sem_unblock);
	k_sem_reset(&sem_done);

	/* Part 1: Timed-out waiter readied behind owner with stale mutex_pended_on */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(PRIO_MID));
	zassert_ok(k_mutex_lock(&mutex_a, K_FOREVER));

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_b_timeout_on_a, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);
	k_sem_take(&sem_low_ready, K_FOREVER);

	/*
	 * Busy-wait until t_low's 5 ms timeout expires in ISR. Because main
	 * is running at equal priority (PRIO_MID), t_low is readied behind
	 * main without preempting it, leaving t_low.mutex_pended_on == &mutex_a
	 * stale while t_low still holds mutex_b and main still holds mutex_a.
	 */
	while ((t_low.base.thread_state & _THREAD_PENDING) != 0U) {
		k_busy_wait(100);
	}
	zassert_equal(t_low.mutex_pended_on, &mutex_a,
		      "expected stale mutex_pended_on before t_low runs");

	/*
	 * Main now locks mutex_b with K_FOREVER while holding mutex_a and
	 * while t_low.mutex_pended_on still points to mutex_a. Must NOT fire
	 * a false-positive deadlock __ASSERT; instead main pends on mutex_b,
	 * allowing t_low to run, clear mutex_pended_on, and unlock mutex_b.
	 */
	zassert_ok(k_mutex_lock(&mutex_b, K_FOREVER));
	zassert_is_null(t_low.mutex_pended_on,
			"t_low should have cleared mutex_pended_on after running");
	zassert_ok(k_mutex_unlock(&mutex_b));
	zassert_ok(k_mutex_unlock(&mutex_a));

	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_low, K_FOREVER);

	/* Part 2: Chain owner pending K_FOREVER on non-mutex wait_q with stale mutex_pended_on */
	zassert_ok(k_mutex_lock(&mutex_a, K_FOREVER));

	k_thread_create(&t_low, stack_low, STACK_SIZE,
			t_b_pend_on_sem_with_stale_mutex, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_MID), 0, K_NO_WAIT);
	k_sem_take(&sem_low_ready, K_FOREVER);

	/* Schedule timer to release t_low from sem_unblock after main pends on mutex_b */
	k_timer_start(&unblock_timer, K_MSEC(10), K_NO_WAIT);

	/*
	 * Must NOT fire false-positive deadlock __ASSERT even though t_low
	 * holds mutex_b, is _THREAD_PENDING with K_FOREVER, and has
	 * t_low.mutex_pended_on == &mutex_a (owned by _current).
	 */
	zassert_ok(k_mutex_lock(&mutex_b, K_FOREVER));
	zassert_ok(k_mutex_unlock(&mutex_b));
	zassert_ok(k_mutex_unlock(&mutex_a));

	k_sem_take(&sem_done, K_FOREVER);
	k_thread_join(&t_low, K_FOREVER);
	k_thread_priority_set(k_current_get(), orig_prio);
#else
	ztest_test_skip();
#endif
}

ZTEST_SUITE(mutex_deadlock, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

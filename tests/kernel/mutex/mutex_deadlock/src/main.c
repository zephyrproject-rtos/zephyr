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
static K_SEM_DEFINE(sem_done,      0, 1);

static K_THREAD_STACK_DEFINE(stack_low, STACK_SIZE);
static struct k_thread t_low;

static void t_b_deadlock(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&mutex_b, K_FOREVER);
	k_sem_give(&sem_low_ready);
	k_mutex_lock(&mutex_a, K_FOREVER);
	k_mutex_unlock(&mutex_a);
	k_mutex_unlock(&mutex_b);
	k_sem_give(&sem_done);
}

#if defined(CONFIG_MUTEX_DEADLOCK_DETECT) && Z_MUTEX_PI_ENABLED
/*
 * After test_deadlock_detection fires __ASSERT, the test function is
 * aborted before it can clean up. This hook runs after the fatal error
 * is caught and restores a clean state (only ztest framework bookkeeping
 * happens after this point in this binary).
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

	/* Neither mutex was ever unlocked; clear owner before re-init. */
	mutex_a.owner = NULL;
	mutex_b.owner = NULL;

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

ZTEST_SUITE(mutex_deadlock, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

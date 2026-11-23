/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cross-CPU mutex priority inheritance test
 *
 * Verifies that a priority boost reaches a mutex owner that is genuinely
 * executing on another core, not just one that has been scheduled out in
 * favor of the waiter. This requires two real CPUs running concurrently
 * (CONFIG_MP_MAX_NUM_CPUS=2, CONFIG_SCHED_CPU_MASK=y), which the other
 * mutex test suites in this tree are not written to tolerate -- it stays
 * in its own test app rather than sharing a binary with them.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS > 1);

#define STACK_SIZE  (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

/* Priority levels */
#define PRIO_HIGH    1
#define PRIO_LOW     5

static K_MUTEX_DEFINE(mutex_a);

static K_SEM_DEFINE(sem_owner_running, 0, 1);
static K_SEM_DEFINE(sem_done,          0, 2);

static K_THREAD_STACK_DEFINE(stack_owner,  STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_waiter, STACK_SIZE);
static struct k_thread t_owner, t_waiter;

static atomic_t owner_spin_count;
static atomic_t stop_owner;

static void t_owner_fn(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&mutex_a, K_FOREVER);
	k_sem_give(&sem_owner_running);

	/*
	 * Busy-spin instead of blocking, so this thread is genuinely
	 * executing on its own core while t_waiter contends for mutex_a.
	 */
	while (!atomic_get(&stop_owner)) {
		atomic_inc(&owner_spin_count);
	}

	k_mutex_unlock(&mutex_a);
	k_sem_give(&sem_done);
}

static void t_waiter_fn(void *p1, void *p2, void *p3)
{
	k_mutex_lock(&mutex_a, K_FOREVER);
	k_mutex_unlock(&mutex_a);
	k_sem_give(&sem_done);
}

/**
 * @brief Verify priority boost reaches an owner actively running on another core
 *
 * T_owner is pinned to CPU 1 and holds mutex_a while busy-spinning, so it
 * is genuinely running rather than blocked. T_waiter is pinned to CPU 0 and
 * blocks on mutex_a at a higher priority, which must boost T_owner while it
 * continues to execute concurrently on CPU 1. Releasing the mutex then
 * hands ownership to T_waiter across the same core boundary.
 */
ZTEST(mutex_smp, test_cross_cpu_priority_boost)
{
#if Z_MUTEX_PI_ENABLED
	int spins_before;

	atomic_set(&stop_owner, 0);
	atomic_set(&owner_spin_count, 0);
	k_sem_reset(&sem_owner_running);
	k_sem_reset(&sem_done);

	k_thread_create(&t_owner, stack_owner, STACK_SIZE,
			t_owner_fn, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_LOW), 0, K_FOREVER);
	k_thread_cpu_pin(&t_owner, 1);
	k_thread_start(&t_owner);

	k_sem_take(&sem_owner_running, K_FOREVER);
	spins_before = atomic_get(&owner_spin_count);

	k_thread_create(&t_waiter, stack_waiter, STACK_SIZE,
			t_waiter_fn, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_FOREVER);
	k_thread_cpu_pin(&t_waiter, 0);
	k_thread_start(&t_waiter);

	/* Give the waiter time to block and the boost to land. */
	k_sleep(K_MSEC(50));

	zassert_true(atomic_get(&owner_spin_count) > spins_before,
		     "owner thread was not actually running concurrently");
	zassert_equal(t_owner.base.prio, PRIO_HIGH,
		      "owner not boosted to waiter's priority while running "
		      "on another core (got %d)", t_owner.base.prio);

	atomic_set(&stop_owner, 1);

	k_sem_take(&sem_done, K_FOREVER);
	k_sem_take(&sem_done, K_FOREVER);

	k_thread_join(&t_owner, K_FOREVER);
	k_thread_join(&t_waiter, K_FOREVER);
#else
	ztest_test_skip();
#endif /* Z_MUTEX_PI_ENABLED */
}

ZTEST_SUITE(mutex_smp, NULL, NULL, NULL, NULL, NULL);

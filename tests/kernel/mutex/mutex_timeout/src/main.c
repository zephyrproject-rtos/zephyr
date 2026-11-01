/*
 * Copyright (c) Rishav Chakraborty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Owner priority deboost when a mutex waiter times out
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define TIMEOUT_MS 200

/* Priority levels */
#define PRIO_HIGH 1
#define PRIO_LOW  5

/* Shared mutexes */
static K_MUTEX_DEFINE(mutex_a);

/* Semaphores for thread synchronization */
static K_SEM_DEFINE(sem_low_ready, 0, 1);

/* Thread objects */
static K_THREAD_STACK_DEFINE(stack_low, STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_high, STACK_SIZE);

static struct k_thread t_low, t_high;

static void t_owner(void *p1, void *p2, void *p3)
{
	volatile bool *flag = (volatile bool *)p1;

	k_mutex_lock(&mutex_a, K_FOREVER);
	k_sem_give(&sem_low_ready);
	while (!*flag) {
		k_busy_wait(100);
	}
	k_mutex_unlock(&mutex_a);
}

static void t_waiter(void *p1, void *p2, void *p3)
{
	bool *flag = (bool *)p1;
	k_timeout_t timeout = *(k_timeout_t *)p2;

	int rc = k_mutex_lock(&mutex_a, timeout);

	zassert_equal(rc, -EAGAIN, "waiter got the mutex");
	*flag = true;
}

/**
 * @brief Verify the owner is deboosted when a waiter's lock times out
 *
 * T_low holds mutex_a and spins at T_high's boosted priority while T_high
 * waits with a timeout. After T_high times out, T_low must be back at
 * PRIO_LOW and T_high must have run to completion.
 */
ZTEST(mutex_timeout, test_owner_deboost_on_lock_timeout)
{
	static k_timeout_t timeout = K_MSEC(TIMEOUT_MS);
	bool flag = false;

	k_thread_create(&t_low, stack_low, STACK_SIZE, t_owner, &flag, NULL, NULL,
			K_PRIO_PREEMPT(PRIO_LOW), 0, K_NO_WAIT);

	k_sem_take(&sem_low_ready, K_FOREVER);

	k_thread_create(&t_high, stack_high, STACK_SIZE, t_waiter, &flag, &timeout, NULL,
			K_PRIO_PREEMPT(PRIO_HIGH), 0, K_NO_WAIT);

	/* Sleep past T_high's timeout so its handler has certainly run. */
	k_sleep(K_MSEC(TIMEOUT_MS + 50));

	zassert_equal(k_thread_priority_get(&t_low), PRIO_LOW,
		      "T_low still boosted after T_high timed out");

	k_thread_join(&t_low, K_FOREVER);
	k_thread_join(&t_high, K_FOREVER);
	zassert_equal(flag, true, "T_high did not finish");
}

static void cleanup(void *unused)
{
	ARG_UNUSED(unused);

	k_thread_abort(&t_low);
	k_thread_abort(&t_high);
}

ZTEST_SUITE(mutex_timeout, NULL, NULL, NULL, cleanup, NULL);

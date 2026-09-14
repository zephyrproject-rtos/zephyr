/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/condvar.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/ztest.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define NUM_WAITERS 3

#ifdef CONFIG_USERSPACE
#define ZTEST_USER_OR_NOT ZTEST_USER
#define WAITER_OPTIONS    (K_USER | K_INHERIT_PERMS)
#else
#define ZTEST_USER_OR_NOT ZTEST
#define WAITER_OPTIONS    0
#endif

static K_THREAD_STACK_ARRAY_DEFINE(waiter_stack, NUM_WAITERS, STACK_SIZE);
static struct k_thread waiter_thread[NUM_WAITERS];

static ZTEST_BMEM SYS_MUTEX_DEFINE(mutex);
static ZTEST_BMEM SYS_CONDVAR_DEFINE(condvar);
static ZTEST_DMEM SYS_CONDVAR_DEFINE_EXT(mono_condvar, SYS_CLOCK_MONOTONIC);
static ZTEST_BMEM int ready;
static ZTEST_BMEM int woken;

static void waiter(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_ok(sys_mutex_lock(&mutex, K_FOREVER));
	while (ready == 0) {
		zassert_ok(sys_condvar_wait(&condvar, &mutex, K_FOREVER));
	}
	woken++;
	zassert_ok(sys_mutex_unlock(&mutex));
}

static void start_waiters(int n)
{
	for (int i = 0; i < n; ++i) {
		k_thread_create(&waiter_thread[i], waiter_stack[i], STACK_SIZE, waiter, NULL, NULL,
				NULL, K_PRIO_PREEMPT(1), WAITER_OPTIONS, K_NO_WAIT);
	}
	/* let every waiter block */
	k_msleep(50);
}

static void join_waiters(int n)
{
	for (int i = 0; i < n; ++i) {
		zassert_ok(k_thread_join(&waiter_thread[i], K_SECONDS(1)));
	}
}

ZTEST_USER_OR_NOT(sys_condvar, test_sys_condvar_init)
{
	struct sys_condvar cv;

	sys_condvar_init(&cv);
	zassert_equal(sys_condvar_clock(&cv), SYS_CLOCK_REALTIME);
	zassert_ok(sys_condvar_destroy(&cv));

	zassert_ok(sys_condvar_init_ext(&cv, SYS_CLOCK_MONOTONIC));
	zassert_equal(sys_condvar_clock(&cv), SYS_CLOCK_MONOTONIC);
	zassert_ok(sys_condvar_destroy(&cv));

	zassert_equal(sys_condvar_init_ext(&cv, 0xff), -EINVAL);

	/* a zero-filled object is a valid condition variable on the default clock */
	cv = (struct sys_condvar){0};
	zassert_equal(sys_condvar_clock(&cv), SYS_CLOCK_REALTIME);
	zassert_equal(sys_condvar_clock(&mono_condvar), SYS_CLOCK_MONOTONIC);
}

ZTEST_USER_OR_NOT(sys_condvar, test_sys_condvar_signal)
{
	ready = 0;
	woken = 0;
	start_waiters(NUM_WAITERS);

	zassert_ok(sys_mutex_lock(&mutex, K_FOREVER));
	ready = 1;
	zassert_ok(sys_condvar_signal(&condvar));
	zassert_ok(sys_mutex_unlock(&mutex));
	k_msleep(50);
	zassert_equal(woken, 1, "signal woke %d waiters", woken);

	for (int i = 1; i < NUM_WAITERS; ++i) {
		zassert_ok(sys_condvar_signal(&condvar));
	}
	join_waiters(NUM_WAITERS);
	zassert_equal(woken, NUM_WAITERS);

	/* a signal with nobody waiting is not an error and is not remembered */
	zassert_ok(sys_condvar_signal(&condvar));
}

ZTEST_USER_OR_NOT(sys_condvar, test_sys_condvar_broadcast)
{
	ready = 0;
	woken = 0;
	start_waiters(NUM_WAITERS);

	zassert_ok(sys_mutex_lock(&mutex, K_FOREVER));
	ready = 1;
	zassert_ok(sys_condvar_broadcast(&condvar));
	zassert_ok(sys_mutex_unlock(&mutex));
	join_waiters(NUM_WAITERS);
	zassert_equal(woken, NUM_WAITERS, "broadcast woke %d waiters", woken);
}

ZTEST_USER_OR_NOT(sys_condvar, test_sys_condvar_wait)
{
	int64_t start;

	zassert_ok(sys_mutex_lock(&mutex, K_FOREVER));

	start = k_uptime_get();
	zassert_equal(sys_condvar_wait(&condvar, &mutex, K_MSEC(20)), -EAGAIN);
	zassert_true(k_uptime_get() - start >= 20);
	/* the mutex is held again after a timeout */
	zassert_equal(sys_mutex_lock(&mutex, K_NO_WAIT), 0, "recursive relock");
	zassert_ok(sys_mutex_unlock(&mutex));

	zassert_equal(sys_condvar_wait(&condvar, &mutex, K_NO_WAIT), -EAGAIN);
	zassert_ok(sys_mutex_unlock(&mutex));

	/* waiting requires the mutex to be locked */
	zassert_equal(sys_condvar_wait(&condvar, &mutex, K_NO_WAIT), -EINVAL);
}

ZTEST_USER_OR_NOT(sys_condvar, test_sys_condvar_clockwait)
{
	struct timespec abstime;

	zassert_ok(sys_mutex_lock(&mutex, K_FOREVER));

	zassert_ok(sys_clock_gettime(SYS_CLOCK_MONOTONIC, &abstime));
	abstime.tv_nsec += 20 * NSEC_PER_MSEC;
	if (abstime.tv_nsec >= NSEC_PER_SEC) {
		abstime.tv_nsec -= NSEC_PER_SEC;
		abstime.tv_sec++;
	}
	zassert_equal(sys_condvar_clockwait(&condvar, &mutex, SYS_CLOCK_MONOTONIC, &abstime),
		      -ETIMEDOUT);
	/* the condition variable's own clock */
	zassert_equal(sys_condvar_timedwait(&mono_condvar, &mutex, &abstime), -ETIMEDOUT);

	zassert_ok(sys_clock_gettime(SYS_CLOCK_REALTIME, &abstime));
	zassert_equal(sys_condvar_timedwait(&condvar, &mutex, &abstime), -ETIMEDOUT);

	zassert_equal(sys_condvar_clockwait(&condvar, &mutex, 0xff, &abstime), -EINVAL);
	abstime.tv_nsec = -1;
	zassert_equal(sys_condvar_clockwait(&condvar, &mutex, SYS_CLOCK_REALTIME, &abstime),
		      -EINVAL);

	zassert_ok(sys_mutex_unlock(&mutex));
}

static void *setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &waiter_thread[0], &waiter_thread[1],
			      &waiter_thread[2], &waiter_stack[0], &waiter_stack[1],
			      &waiter_stack[2]);
#endif
	return NULL;
}

ZTEST_SUITE(sys_condvar, NULL, setup, NULL, NULL, NULL);

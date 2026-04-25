/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/thread.h>
#include <zephyr/ztest.h>


static void thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
}

ZTEST_USER(sys_thread, test_thread_pool)
{
	int i;
	struct k_thread *th[SYS_THREAD_THREAD_MIN + 1];

	for (i = 0; i < SYS_THREAD_THREAD_MIN; ++i) {
		zassert_ok(sys_thread_create(&th[i], NULL, 0, 0, thread_entry, NULL, NULL, NULL,
					K_LOWEST_APPLICATION_THREAD_PRIO,
					K_INHERIT_PERMS));
	}

	if (CONFIG_SYS_THREAD_THREAD_MAX == SYS_THREAD_THREAD_MIN) {
		/* if min and max are the same, only statically allocated threads are in the pool */
		zassert_equal(-ENOMEM, sys_thread_create(&th[i], NULL, 0, 0, thread_entry, NULL, NULL, NULL,
					K_LOWEST_APPLICATION_THREAD_PRIO, 0));
	} else {
		/* allocate one more from the heap */
		zassert_ok(sys_thread_create(&th[i], NULL, 0, 0, thread_entry, NULL, NULL, NULL,
					K_LOWEST_APPLICATION_THREAD_PRIO, 0));
		++i;
	}

	for (--i; i >= 0; --i) {
		zassert_ok(k_thread_join(th[i], K_FOREVER));
	}
}

static ZTEST_BMEM sys_thread_once_t once = SYS_THREAD_ONCE_INIT;
static ZTEST_BMEM int once_count;
static ZTEST_BMEM bool once_started;

static void once_func(void)
{
	++once_count;
}

static void once_slow(void)
{
	once_started = true;
	k_msleep(20);
	++once_count;
}

static void once_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sys_thread_once(&once, (void (*)(void))p1);
}

static void once_reset(void)
{
	once = (sys_thread_once_t)SYS_THREAD_ONCE_INIT;
	once_count = 0;
	once_started = false;
}

static void once_single(void)
{
	struct k_thread *th;

	once_reset();

	sys_thread_once(&once, once_func);
	sys_thread_once(&once, once_func);

	zassert_ok(sys_thread_create(&th, NULL, 0, 0, once_entry, once_func, NULL, NULL,
				     K_LOWEST_APPLICATION_THREAD_PRIO, K_INHERIT_PERMS));
	zassert_ok(k_thread_join(th, K_FOREVER));

	zassert_equal(once_count, 1, "init routine ran %d times", once_count);
}

static void once_contended(void)
{
	struct k_thread *th;

	once_reset();

	zassert_ok(sys_thread_create(&th, NULL, 0, 0, once_entry, once_slow, NULL, NULL,
				     K_LOWEST_APPLICATION_THREAD_PRIO, K_INHERIT_PERMS));
	while (!once_started) {
		k_msleep(1);
	}

	/* must block until the initializer in th completes */
	sys_thread_once(&once, once_slow);
	zassert_equal(once_count, 1, "returned before init routine completed");

	zassert_ok(k_thread_join(th, K_FOREVER));
	zassert_equal(once_count, 1, "init routine ran %d times", once_count);
}

ZTEST_USER(sys_thread, test_once)
{
	once_single();
	once_contended();
}

static void sleep_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(K_FOREVER);
}

/* detached pooled threads return to the pool on their own: the pool fills again */
ZTEST_USER(sys_thread, test_thread_pool_detached)
{
	struct k_thread *th[SYS_THREAD_THREAD_MIN];

	if (SYS_THREAD_THREAD_MIN == 0) {
		ztest_test_skip();
	}

	for (int round = 0; round < 3; ++round) {
		for (int i = 0; i < SYS_THREAD_THREAD_MIN; ++i) {
			zassert_ok(sys_thread_create(&th[i], NULL, 0, 0, thread_entry, NULL, NULL,
						     NULL, K_LOWEST_APPLICATION_THREAD_PRIO,
						     K_INHERIT_PERMS),
				   "round %d: thread %d", round, i);
			zassert_ok(k_thread_detach(th[i]));
		}
		/* let them run and exit; the next allocation reclaims them */
		k_msleep(20);
	}
}

/* threads aborted by another thread are parked, and their join reclaims them */
ZTEST_USER(sys_thread, test_thread_pool_abort)
{
	struct k_thread *th[SYS_THREAD_THREAD_MIN];

	if (SYS_THREAD_THREAD_MIN == 0) {
		ztest_test_skip();
	}

	for (int round = 0; round < 3; ++round) {
		for (int i = 0; i < SYS_THREAD_THREAD_MIN; ++i) {
			zassert_ok(sys_thread_create(&th[i], NULL, 0, 0, sleep_entry, NULL, NULL,
						     NULL, K_LOWEST_APPLICATION_THREAD_PRIO,
						     K_INHERIT_PERMS),
				   "round %d: thread %d", round, i);
		}
		k_msleep(5);
		for (int i = 0; i < SYS_THREAD_THREAD_MIN; ++i) {
			k_thread_abort(th[i]);
		}
		for (int i = 0; i < SYS_THREAD_THREAD_MIN; ++i) {
			zassert_ok(k_thread_join(th[i], K_NO_WAIT));
		}
	}
}

ZTEST_SUITE(sys_thread, NULL, NULL, NULL, NULL, NULL);

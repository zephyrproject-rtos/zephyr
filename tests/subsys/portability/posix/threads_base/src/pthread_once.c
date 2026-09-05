/*
 * Copyright (c) 2026 Göthel Software
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/atomic_types.h>

/* #define DEBUG_OUTPUT 1 */

#ifdef DEBUG_OUTPUT

#define dbg_PRINT(fmt, ...)                                                                        \
	{                                                                                          \
		char name[20];                                                                     \
                                                                                                   \
		pthread_getname_np(pthread_self(), name, sizeof(name));                            \
		fputs(name, stderr);                                                               \
		fputs(": ", stderr);                                                               \
		fprintf(stderr, (fmt)__VA_OPT__(,) __VA_ARGS__);                                   \
		fflush(stderr);                                                                    \
	}

#else

#define dbg_PRINT(fmt, ...)

#endif

#define ONCE_INIT_VALUE 0
static atomic_t once_value = ATOMIC_INIT(0);
static const atomic_val_t once_init_value = ONCE_INIT_VALUE;
static pthread_once_t once_control2 = PTHREAD_ONCE_INIT;

static void InitFunc(void)
{
	atomic_val_t o = atomic_get(&once_value);
	(void)o;
	dbg_PRINT("XXXX %ld -> %ld ...\n", o, o + 1);

	/* trigger race condition in pthread_once if not waiting for init-func completion. */
	k_yield();

	atomic_inc(&once_value);
	dbg_PRINT("XXXX %ld -> %ld done\n", o, o + 1);
}

ZTEST(pthread_once, test_0_single)
{
	pthread_setname_np(pthread_self(), "thread_M");
	atomic_set(&once_value, 0);
	static pthread_once_t once_control = PTHREAD_ONCE_INIT;

	/* 1st call */
	zassert_ok(pthread_once(&once_control, &InitFunc));
	zassert_equal(once_init_value + 1, atomic_get(&once_value));

	/* 2nd call */
	zassert_ok(pthread_once(&once_control, &InitFunc));
	zassert_equal(once_init_value + 1, atomic_get(&once_value));
}

ZTEST(pthread_once, test_1_badvalue)
{
	pthread_setname_np(pthread_self(), "thread_M");
	atomic_set(&once_value, 0);
	static pthread_once_t once_control = {99}; /* bad value */

	zassert_equal(EINVAL, pthread_once(&once_control, &InitFunc));
}

#define MY_MIN(a, b)    ((a) < (b) ? (a) : (b))
#define MY_THREAD_COUNT (MY_MIN(CONFIG_POSIX_THREAD_THREADS_MAX, CONFIG_MAX_PTHREAD_COND_COUNT) - 2)
static pthread_mutex_t m_tstart = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv_tstart;
static pthread_cond_t cv_run[MY_THREAD_COUNT];

static void *AThread(void *arg)
{
	int *id_temp = (int *)arg;
	const int id = *id_temp;

	/* save handling of arg lifecycle */
	zassert_ok(pthread_mutex_lock(&m_tstart));
	{
		char name[20];

		snprintf(name, 20, "thread_%02d", id);
		pthread_setname_np(pthread_self(), name);
	}
	zassert_ok(pthread_cond_signal(&cv_tstart));

	/* post thread creation test launch */
	zassert_ok(pthread_cond_wait(&cv_run[id], &m_tstart));
	zassert_ok(pthread_mutex_unlock(&m_tstart) /* NOSONAR */);
	dbg_PRINT("begin\n");

	/* warm up scheduler */
	for (int i = 0; i < 4; ++i) {
		k_yield();
	}
	zassert_ok(pthread_once(&once_control2, &InitFunc));
	const atomic_val_t v = atomic_get(&once_value);

	zassert_equal(once_init_value + 1, v);

	dbg_PRINT("end\n");
	return NULL;
}

ZTEST(pthread_once, test_2_parallel)
{
	pthread_t threads[MY_THREAD_COUNT];

	pthread_setname_np(pthread_self(), "thread_M");
	pthread_cond_init(&cv_tstart, NULL);
	for (int i = 0; i < MY_THREAD_COUNT; ++i) {
		pthread_cond_init(&cv_run[i], NULL);
	}
	atomic_set(&once_value, 0);
	dbg_PRINT("begin\n");

	zassert_ok(pthread_mutex_lock(&m_tstart));
	for (int i = 0; i < MY_THREAD_COUNT; ++i) {
		/* save handling of arg lifecycle */
		zassert_ok(pthread_create(&threads[i], NULL, &AThread, &i));
		zassert_ok(pthread_cond_wait(&cv_tstart, &m_tstart));
	}
	for (int i = 0; i < MY_THREAD_COUNT; ++i) {
		zassert_ok(pthread_cond_signal(&cv_run[i]));
	}
	dbg_PRINT("run\n");
	zassert_ok(pthread_mutex_unlock(&m_tstart) /* NOSONAR */);

	for (int i = 0; i < MY_THREAD_COUNT; ++i) {
		zassert_ok(pthread_join(threads[i], NULL));
	}
	pthread_cond_destroy(&cv_tstart);
	for (int i = 0; i < MY_THREAD_COUNT; ++i) {
		pthread_cond_destroy(&cv_run[i]);
	}
	const atomic_val_t v = atomic_get(&once_value);

	dbg_PRINT("once_value %ld\n", v);
	zassert_equal(once_init_value + 1, v);
	dbg_PRINT("end\n");
}

static void before(void *arg)
{
	ARG_UNUSED(arg);
	dbg_PRINT("CONFIG_POSIX_THREAD_THREADS_MAX %d\n", CONFIG_POSIX_THREAD_THREADS_MAX);
	dbg_PRINT("CONFIG_MAX_PTHREAD_COND_COUNT %d\n", CONFIG_MAX_PTHREAD_COND_COUNT);
	dbg_PRINT("MY_THREAD_COUNT %d\n", MY_THREAD_COUNT);

	dbg_PRINT("CONFIG_SMP %d\n", IS_ENABLED(CONFIG_SMP));
	dbg_PRINT("CONFIG_MULTITHREADING %d\n", IS_ENABLED(CONFIG_MULTITHREADING));
	dbg_PRINT("CONFIG_TIMESLICE_PER_THREAD %d\n", IS_ENABLED(CONFIG_TIMESLICE_PER_THREAD));
	dbg_PRINT("CONFIG_SYS_CLOCK_TICKS_PER_SEC %d\n", CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	dbg_PRINT("CONFIG_SCHED_DEADLINE %d\n", IS_ENABLED(CONFIG_SCHED_DEADLINE));

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(pthread_once, NULL, NULL, before, NULL, NULL);

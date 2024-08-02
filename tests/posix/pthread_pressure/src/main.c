/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define STACK_SIZE K_THREAD_STACK_LEN(CONFIG_TEST_STACK_SIZE)

/* update interval for printing stats */
#if CONFIG_TEST_DURATION_S >= 60
#define UPDATE_INTERVAL_S 10
#elif CONFIG_TEST_DURATION_S >= 30
#define UPDATE_INTERVAL_S 5
#else
#define UPDATE_INTERVAL_S 1
#endif

/* 32 threads is mainly a limitation of find_lsb_set() */
#define NUM_THREADS MIN(32, MIN(CONFIG_TEST_NUM_CPUS, CONFIG_POSIX_THREAD_THREADS_MAX))

typedef int (*create_fn)(int i);
typedef int (*join_fn)(int i);

static void *setup(void);
static void before(void *fixture);

/* bitmask of available threads */
static bool alive[NUM_THREADS];

/* array of thread stacks */
static K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);

static struct k_thread k_threads[NUM_THREADS];
static uint64_t counters[NUM_THREADS];
static uint64_t prev_counters[NUM_THREADS];

static void print_stats(uint64_t now, uint64_t end)
{
	printk("now (ms): %llu end (ms): %llu\n", now, end);
	for (int i = 0; i < NUM_THREADS; ++i) {
		printk("Thread %d created and joined %llu times (%llu joins/s)\n", i, counters[i],
		       (counters[i] - prev_counters[i]) / UPDATE_INTERVAL_S);
		prev_counters[i] = counters[i];
	}
}

static void test_create_join_common(const char *tag, create_fn create, join_fn join)
{
	int i;
	int ret;
	uint64_t now_ms = k_uptime_get();
	const uint64_t end_ms = now_ms + MSEC_PER_SEC * CONFIG_TEST_DURATION_S;
	uint64_t update_ms = now_ms + MSEC_PER_SEC * UPDATE_INTERVAL_S;

	printk("BOARD: %s\n", CONFIG_BOARD);
	printk("CONFIG_SMP: %s\n", IS_ENABLED(CONFIG_SMP) ? "y" : "n");
	printk("NUM_THREADS: %u\n", NUM_THREADS);
	printk("TEST_NUM_CPUS: %u\n", CONFIG_TEST_NUM_CPUS);
	printk("TEST_DURATION_S: %u\n", CONFIG_TEST_DURATION_S);
	printk("TEST_DELAY_US: %u\n", CONFIG_TEST_DELAY_US);

	for (i = 0; i < NUM_THREADS; ++i) {
		/* spawn thread i */
		prev_counters[i] = 0;
		ret = create(i);
		if (IS_ENABLED(CONFIG_TEST_EXTRA_ASSERTIONS)) {
			zassert_ok(ret, "%s_create(%d)[%zu] failed: %d", tag, i, counters[i], ret);
		}
	}

	do {
		if (!IS_ENABLED(CONFIG_SMP)) {
			/* allow the test thread to be swapped-out */
			k_yield();
		}

		for (i = 0; i < NUM_THREADS; ++i) {
			if (alive[i]) {
				ret = join(i);
				if (IS_ENABLED(CONFIG_TEST_EXTRA_ASSERTIONS)) {
					zassert_ok(ret, "%s_join(%d)[%zu] failed: %d", tag, i,
						   counters[i], ret);
				}
				alive[i] = false;

				/* update counter i after each (create,join) pair */
				++counters[i];

				if (IS_ENABLED(CONFIG_TEST_DELAY_US)) {
					/* success with 0 delay means we are ~raceless */
					k_busy_wait(CONFIG_TEST_DELAY_US);
				}

				/* re-spawn thread i */
				ret = create(i);
				if (IS_ENABLED(CONFIG_TEST_EXTRA_ASSERTIONS)) {
					zassert_ok(ret, "%s_create(%d)[%zu] failed: %d", tag, i,
						   counters[i], ret);
				}
			}
		}

		/* are we there yet? */
		now_ms = k_uptime_get();

		/* dump some stats periodically */
		if (now_ms > update_ms) {
			update_ms += MSEC_PER_SEC * UPDATE_INTERVAL_S;

			/* at this point, we should have seen many context switches */
			for (i = 0; i < NUM_THREADS; ++i) {
				if (IS_ENABLED(CONFIG_TEST_EXTRA_ASSERTIONS)) {
					zassert_true(counters[i] > 0, "%s %d was never scheduled",
						     tag, i);
				}
			}

			print_stats(now_ms, end_ms);
		}
		Z_SPIN_DELAY(100);
	} while (end_ms > now_ms);

	print_stats(now_ms, end_ms);
}

/*
 * Wrappers for k_threads
 */

static void k_thread_fun(void *arg1, void *arg2, void *arg3)
{
	int i = POINTER_TO_INT(arg1);

	alive[i] = true;
}

static int k_thread_create_wrapper(int i)
{
	k_thread_create(&k_threads[i], thread_stacks[i], STACK_SIZE, k_thread_fun,
			INT_TO_POINTER(i), NULL, NULL, K_HIGHEST_APPLICATION_THREAD_PRIO, 0,
			K_NO_WAIT);

	return 0;
}

static int k_thread_join_wrapper(int i)
{
	return k_thread_join(&k_threads[i], K_FOREVER);
}

ZTEST(pthread_pressure, test_k_thread_create_join)
{
	if (IS_ENABLED(CONFIG_TEST_KTHREADS)) {
		test_create_join_common("k_thread", k_thread_create_wrapper, k_thread_join_wrapper);
	} else {
		ztest_test_skip();
	}
}

/*
 * Wrappers for pthreads
 */

static pthread_t pthreads[NUM_THREADS];
static pthread_attr_t pthread_attrs[NUM_THREADS];

static void *pthread_fun(void *arg)
{
	k_thread_fun(arg, NULL, NULL);
	return NULL;
}

static int pthread_create_wrapper(int i)
{
	return pthread_create(&pthreads[i], &pthread_attrs[i], pthread_fun, INT_TO_POINTER(i));
}

static int pthread_join_wrapper(int i)
{
	return pthread_join(pthreads[i], NULL);
}

ZTEST(pthread_pressure, test_pthread_create_join)
{
	if (IS_ENABLED(CONFIG_TEST_PTHREADS)) {
		test_create_join_common("pthread", pthread_create_wrapper, pthread_join_wrapper);
	} else {
		ztest_test_skip();
	}
}

/*
 * Test suite / fixture
 */

ZTEST_SUITE(pthread_pressure, NULL, setup, before, NULL, NULL);

static void *setup(void)
{
	if (IS_ENABLED(CONFIG_TEST_PTHREADS)) {
		const struct sched_param param = {
			.sched_priority = sched_get_priority_max(SCHED_FIFO),
		};

		/* setup pthread stacks */
		for (int i = 0; i < NUM_THREADS; ++i) {
			zassert_ok(pthread_attr_init(&pthread_attrs[i]));
			zassert_ok(pthread_attr_setstack(&pthread_attrs[i], thread_stacks[i],
							 STACK_SIZE));
			zassert_ok(pthread_attr_setschedpolicy(&pthread_attrs[i], SCHED_FIFO));
			zassert_ok(pthread_attr_setschedparam(&pthread_attrs[i], &param));
		}
	}

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(before);

	for (int i = 0; i < NUM_THREADS; ++i) {
		counters[i] = 0;
	}
}

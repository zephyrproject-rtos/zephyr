/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
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
#define NUM_THREADS MIN(32, MIN(CONFIG_TEST_NUM_CPUS, CONFIG_MAX_PTHREAD_COUNT))

typedef int (*create_fn)(int i);
typedef int (*join_fn)(int i);

static void *setup(void);
static void before(void *fixture);

static LOG_MODULE_REGISTER(pthread_pressure, LOG_LEVEL_INF);

/* bitmask of available threads */
static atomic_t free_threads = ATOMIC_INIT(BIT_MASK(NUM_THREADS));

/* array of thread stacks */
static K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);

static struct k_thread k_threads[NUM_THREADS];
static size_t counters[NUM_THREADS];

static void print_stats(uint64_t now, uint64_t end)
{
	LOG_INF("now: %" PRIu64 " end: %" PRIu64 " (ticks)", now, end);

	for (int i = 0; i < NUM_THREADS; ++i) {
		LOG_INF("Thread %d created and joined %zu times", i, counters[i]);
	}
}

static void test_create_join_common(const char *tag, create_fn create, join_fn join)
{
	int i;
	int ret;
	uint64_t now_ticks;
	uint32_t maybe_free_threads;
	const uint64_t end_ticks =
		sys_clock_timeout_end_calc(K_MSEC(MSEC_PER_SEC * CONFIG_TEST_DURATION_S));
	uint64_t update_ticks =
		sys_clock_timeout_end_calc(K_MSEC(MSEC_PER_SEC * UPDATE_INTERVAL_S));

	LOG_INF("NUM_THREADS: %u", NUM_THREADS);
	LOG_INF("TEST_NUM_CPUS: %u", CONFIG_TEST_NUM_CPUS);
	LOG_INF("TEST_DURATION_S: %u", CONFIG_TEST_DURATION_S);
	LOG_INF("TEST_DELAY_US: %u", CONFIG_TEST_DELAY_US);

	for (i = 0; i < NUM_THREADS; ++i) {
		/* mark thread i as in-use */
		atomic_clear_bit(&free_threads, i);

		/* spawn thread i */
		ret = create(i);
		zassert_ok(ret, "%s_create(%d)[%zu] failed: %d", i, counters[i], ret);
		LOG_DBG("spawned thread %d", i);
	}

	do {
		/* allow the test thread to be swapped-out */
		k_yield();

		/* "maybe": window after bit is set before terminating and becoming joinable */
		maybe_free_threads = (uint32_t)atomic_get(&free_threads);
		LOG_DBG("free_threads: %x", maybe_free_threads);

		while (maybe_free_threads != 0) {

			/* shouldn't matter if we join 0 or N-1 first */
			i = find_lsb_set(maybe_free_threads) - 1;

			/* join thread i */
			LOG_DBG("joining thread %d", i);
			ret = join(i);
			zassert_ok(ret, "%s_join(%d)[%zu] failed: %d", i, counters[i], ret);

			/* update counter i after each (create,join) pair */
			++counters[i];

			/* success with 0 delay means we are ~raceless */
			k_busy_wait(CONFIG_TEST_DELAY_US);

			/* mark thread i as in-use */
			atomic_clear_bit(&free_threads, i);
			maybe_free_threads &= ~BIT(i);

			/* re-spawn thread i */
			ret = create(i);
			zassert_ok(ret, "%s_create(%d)[%zu] failed: %d", i, counters[i], ret);
			LOG_DBG("respawned thread %d", i);
		}

		/* are we there yet? */
		now_ticks = k_uptime_ticks();

		/* dump some stats periodically */
		if (now_ticks > update_ticks) {
			update_ticks = sys_clock_timeout_end_calc(
				K_MSEC(MSEC_PER_SEC * UPDATE_INTERVAL_S));

			/* at this point, we should have seen many context switches */
			for (i = 0; i < NUM_THREADS; ++i) {
				zassert_true(counters[i] > 0, "%s %d was never scheduled", tag, i);
			}

			print_stats(now_ticks, end_ticks);
		}
	} while (end_ticks > now_ticks);

	print_stats(now_ticks, end_ticks);
}

/*
 * Wrappers for k_threads
 */

static void k_thread_fun(void *arg1, void *arg2, void *arg3)
{
	int i = POINTER_TO_INT(arg1);

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	/* mark thread i as free-ish again */
	atomic_set_bit(&free_threads, i);

	LOG_DBG("thread %d exiting..", i);
}

static int k_thread_create_wrapper(int i)
{
	k_thread_create(&k_threads[i], &thread_stacks[i][0], STACK_SIZE, k_thread_fun,
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
	const struct sched_param param = {
		.sched_priority = sched_get_priority_max(SCHED_FIFO),
	};

	/* setup pthread stacks */
	for (int i = 0; i < NUM_THREADS; ++i) {
		zassert_ok(pthread_attr_init(&pthread_attrs[i]));
		zassert_ok(
			pthread_attr_setstack(&pthread_attrs[i], &thread_stacks[i][0], STACK_SIZE));
		zassert_ok(pthread_attr_setschedpolicy(&pthread_attrs[i], SCHED_FIFO));
		zassert_ok(pthread_attr_setschedparam(&pthread_attrs[i], &param));
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

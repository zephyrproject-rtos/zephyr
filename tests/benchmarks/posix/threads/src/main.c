/*
 * Copyright (c) 2023, Meta
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <stdio.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

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
#define NUM_CPUS MIN(32, MIN(CONFIG_MP_MAX_NUM_CPUS, CONFIG_POSIX_THREAD_THREADS_MAX))

typedef int (*create_fn)(int i);
typedef int (*join_fn)(int i);

static void before(void);

/* bitmask of available threads */
static bool alive[NUM_CPUS];

/* array of thread stacks */
static K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_CPUS, STACK_SIZE);

static struct k_thread k_threads[NUM_CPUS];
static uint64_t counters[NUM_CPUS];
static uint64_t prev_counters[NUM_CPUS];

static void print_stats(const char *tag, uint64_t now, uint64_t end)
{
	for (int i = 0; i < NUM_CPUS; ++i) {
		printf("%s, %d, %u, %llu, 1, %llu\n", tag, i, UPDATE_INTERVAL_S, counters[i],
		       (counters[i] - prev_counters[i]) / UPDATE_INTERVAL_S);
		prev_counters[i] = counters[i];
	}
}

static void print_group_stats(const char *tag)
{
	uint64_t count = 0;

	for (int i = 0; i < NUM_CPUS; ++i) {
		count += counters[i];
	}

	printf("%s, ALL, %u, %llu, %u, %llu\n", tag, CONFIG_TEST_DURATION_S, count, NUM_CPUS,
	       count / CONFIG_TEST_DURATION_S / NUM_CPUS);
}

static void create_join_common(const char *tag, create_fn create, join_fn join)
{
	int i;
	int __maybe_unused ret;
	uint64_t now_ms = k_uptime_get();
	const uint64_t end_ms = now_ms + MSEC_PER_SEC * CONFIG_TEST_DURATION_S;
	uint64_t update_ms = now_ms + MSEC_PER_SEC * UPDATE_INTERVAL_S;

	for (i = 0; i < NUM_CPUS; ++i) {
		/* spawn thread i */
		prev_counters[i] = 0;
		ret = create(i);
		__ASSERT(ret == 0, "%s_create(%d)[%zu] failed: %d", tag, i, counters[i], ret);
	}

	do {
		if (!IS_ENABLED(CONFIG_SMP)) {
			/* allow the test thread to be swapped-out */
			k_yield();
		}

		for (i = 0; i < NUM_CPUS; ++i) {
			if (alive[i]) {
				ret = join(i);
				__ASSERT(ret, "%s_join(%d)[%zu] failed: %d", tag, i, counters[i],
					 ret);
				alive[i] = false;

				/* update counter i after each (create,join) pair */
				++counters[i];

				if (IS_ENABLED(CONFIG_TEST_DELAY_US)) {
					/* success with 0 delay means we are ~raceless */
					k_busy_wait(CONFIG_TEST_DELAY_US);
				}

				/* re-spawn thread i */
				ret = create(i);
				__ASSERT(ret == 0, "%s_create(%d)[%zu] failed: %d", tag, i,
					 counters[i], ret);
			}
		}

		/* are we there yet? */
		now_ms = k_uptime_get();

		/* dump some stats periodically */
		if (now_ms > update_ms) {
			update_ms += MSEC_PER_SEC * UPDATE_INTERVAL_S;

			/* at this point, we should have seen many context switches */
			for (i = 0; IS_ENABLED(CONFIG_ASSERT) && i < NUM_CPUS; ++i) {
				__ASSERT(counters[i] > 0, "%s %d was never scheduled", tag, i);
			}

			if (IS_ENABLED(CONFIG_TEST_PERIODIC_STATS)) {
				print_stats(tag, now_ms, end_ms);
			}
		}
		Z_SPIN_DELAY(100);
	} while (end_ms > now_ms);

	print_group_stats(tag);
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

static void create_join_kthread(void)
{
	if (IS_ENABLED(CONFIG_TEST_KTHREADS)) {
		before();
		create_join_common("k_thread", k_thread_create_wrapper, k_thread_join_wrapper);
	}
}

/*
 * Wrappers for pthreads
 */

static pthread_t pthreads[NUM_CPUS];
static pthread_attr_t pthread_attrs[NUM_CPUS];

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

static void create_join_pthread(void)
{
	if (IS_ENABLED(CONFIG_TEST_PTHREADS)) {
		before();
		create_join_common("pthread", pthread_create_wrapper, pthread_join_wrapper);
	}
}

static void setup(void)
{
	printf("ASSERT: %c\n", IS_ENABLED(CONFIG_ASSERT) ? 'y' : 'n');
	printf("BOARD: %s\n", CONFIG_BOARD);
	printf("NUM_CPUS: %u\n", NUM_CPUS);
	printf("TEST_DELAY_US: %u\n", CONFIG_TEST_DELAY_US);
	printf("TEST_DURATION_S: %u\n", CONFIG_TEST_DURATION_S);
	printf("SMP: %c\n", IS_ENABLED(CONFIG_SMP) ? 'y' : 'n');

	printf("API, Thread ID, time(s), threads, cores, rate (threads/s/core)\n");

	if (IS_ENABLED(CONFIG_TEST_PTHREADS)) {
		int __maybe_unused ret;
		const struct sched_param param = {
			.sched_priority = sched_get_priority_max(SCHED_FIFO),
		};

		/* setup pthread stacks */
		for (int i = 0; i < NUM_CPUS; ++i) {
			ret = pthread_attr_init(&pthread_attrs[i]);
			__ASSERT(ret == 0, "pthread_attr_init[%d] failed: %d", i, ret);

			ret = pthread_attr_setstack(&pthread_attrs[i], thread_stacks[i],
						    STACK_SIZE);
			__ASSERT(ret == 0, "pthread_attr_setstack[%d] failed: %d", i, ret);

			ret = pthread_attr_setschedpolicy(&pthread_attrs[i], SCHED_FIFO);
			__ASSERT(ret == 0, "pthread_attr_setschedpolicy[%d] failed: %d", i, ret);

			ret = pthread_attr_setschedparam(&pthread_attrs[i], &param);
			__ASSERT(ret == 0, "pthread_attr_setschedparam[%d] failed: %d", i, ret);
		}
	}
}

static void before(void)
{
	for (int i = 0; i < NUM_CPUS; ++i) {
		counters[i] = 0;
	}
}

int main(void)
{
	setup();

	create_join_kthread();
	create_join_pthread();

	printf("PROJECT EXECUTION SUCCESSFUL\n");
}

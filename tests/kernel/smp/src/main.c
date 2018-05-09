/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <ksched.h>

#if 0
#if CONFIG_MP_NUM_CPUS < 2
#error SMP test requires at least two CPUs!
#endif
#endif

#define DELAY_US 50000
#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACKSIZE)
#define THREADS_NUM 5
#define TIME_SLICE_MS 25
#define THREAD_START_DELAY_MS 50

K_THREAD_STACK_DEFINE(t2_stack, STACK_SIZE);
struct k_thread t2;
volatile int t2_count;

static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREADS_NUM, STACK_SIZE);
static struct k_thread tthread[THREADS_NUM];

struct tthread_data {
	k_tid_t tid;
	int priority;
	int executed;
};

volatile struct tthread_data tdata[THREADS_NUM];


void t2_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	t2_count = 0;

	/* This thread simply increments a counter while spinning on
	 * the CPU.  The idea is that it will always be iterating
	 * faster than the other thread so long as it is fairly
	 * scheduled (and it's designed to NOT be fairly schedulable
	 * without a separate CPU!), so the main thread can always
	 * check its progress.
	 */

	while (1) {
		k_busy_wait(DELAY_US);
		t2_count++;
	}
}

/**
 * @brief Verify multi core execution with cooperative threads
 *
 * @details Create a cooperative thread, since current thread is
 * also cooperative, there is no chance that this thread
 * will get scheduled in uniprocessor mode.
 *
 */
void test_smp_coop_threads(void)
{
	/* In a multi processor mode, this thread should have
	 * been scheduled on another core other than the one
	 * which has test_main scheduled
	 */
	int i, ok = 1;

	k_tid_t tid = k_thread_create(&t2, t2_stack, STACK_SIZE, t2_fn,
				      NULL, NULL, NULL,
				      K_PRIO_COOP(1), 0, K_NO_WAIT);

	/* Wait for the other thread (on a separate CPU) to actually
	 * start running.  We want synchrony to be as perfect as
	 * possible.
	 */
	t2_count = -1;
	while (t2_count == -1) {
	}

	for (i = 0; i < 10; i++) {
		k_busy_wait(DELAY_US + (DELAY_US / 8));

		if (t2_count <= i) {
			ok = 0;
			break;
		}
	}

	k_thread_abort(tid);

	zassert_true(ok == 1, "SMP failed");
}

static void smp_thread_entry(void *p1, void *p2, void *p3)
{
	int tnum = (int)p1;

	k_busy_wait(DELAY_US);

	tdata[tnum].executed = 1;
}


/**
 * @brief Verify round robin scheduling on all cores
 *
 * @details The test verifies a scenario in which
 * parent thread joins children to complete task in
 * round robin.
 */
static void test_round_robin_threads(void)
{
	int i, fail = 0;

	/* Make current thread preemptive */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(10));

	k_sched_time_slice_set(TIME_SLICE_MS, 0);

	/* Spawn threads of priority equal to
	 * current thread */
	for (i = 0; i < THREADS_NUM; i++) {
		tdata[i].executed = 0;
		tdata[i].tid = k_thread_create(&tthread[i], tstack[i],
					       STACK_SIZE, smp_thread_entry,
					       (void *)i, NULL, NULL,
					       K_PRIO_PREEMPT(10), 0, 0);
	}

	k_sleep((DELAY_US * THREADS_NUM) / CONFIG_MP_NUM_CPUS);

	for (i = 0; i < THREADS_NUM; i++) {
		k_thread_abort(tdata[i].tid);
	}

	k_sched_time_slice_set(0, 0);
	/* Test point: All THREADS_NUM spawned threads must have
	 * executed.
	 */
	for (i = 0; i < THREADS_NUM; i++) {
		if (tdata[i].executed != 1) {
			fail = 1;
			break;
		}
	}
	zassert_true(fail == 0, "Round robin fails");
}

void test_main(void)
{
	/* Sleep a bit to guarantee that both CPUs enter an idle
	 * thread from which they can exit correctly to run the main
	 * test.
	 */
	k_sleep(1000);

	ztest_test_suite(smp,
			 ztest_unit_test(test_smp_coop_threads),
			 ztest_unit_test(test_round_robin_threads)
			 );
	ztest_run_test_suite(smp);
}

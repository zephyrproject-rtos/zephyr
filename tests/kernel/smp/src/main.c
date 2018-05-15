/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <kernel.h>
#include <kernel_structs.h>

#if CONFIG_MP_NUM_CPUS < 2
#error SMP test requires at least two CPUs!
#endif

#define THREAD_DELAY 200
#define DELAY_US 50000
#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACKSIZE)
#define THREADS_NUM 4

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

static struct tthread_data tdata[THREADS_NUM];

#define DEBUG_ENABLE 1

#if !DEBUG_ENABLE
#define TC_PRINT(...) 0
#endif

void t2_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

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

void test_smp_preemp_threads(void)
{
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(1));

	/* Create a thread at a fixed priority lower than the current
	 * thread and greater than test_main thread. So it preempts
	 * test_main. In uniprocessor mode, this thread will never be
	 * scheduled and the test will fail.
	 */

	k_tid_t tid = k_thread_create(&t2, t2_stack, STACK_SIZE, t2_fn,
				      NULL, NULL, NULL,
				      K_PRIO_PREEMPT(2), 0, K_NO_WAIT);

	for (int i = 0; i < 10; i++) {

		/* Wait slightly longer than the other thread so our
		 * count will always be lower
		 */
		k_busy_wait(DELAY_US + (DELAY_US / 8));

		zassert_true(t2_count > i, "T2 thread isn't"
			     " scheduled to run on other core");
	}
	/* Abort the t2_fn thread */
	k_thread_abort(tid);

	/* At this point of time, 1 core as current thread running,
	 * and other core has test_main thread running.
	 * Once current thread is terminated, test_main reports the
	 * test suite execution pass/fail.
	 */
}

void test_smp_coop_threads(void)
{
	/* In a multi processor mode, this thread should have
	 * been scheduled on another core other than the one
	 * which has test_main scheduled
	 */
	int i;

	/* Create a cooperative thread, since current thread is
	 * also cooperative, there is no chance that this thread
	 * will get scheduled in uniprocessor mode.
	 *
	 * For ESP32 only (has only 2 cores): Since t2 thread has
	 * higher priority than test_main and test_main is preemptive,
	 * test_main should be moved to timeout queue and t2
	 * thread should get scheduled instead. So t2 thread should
	 * be in one core and test_smp_coop_threads should be
	 * another core.
	 */
	k_tid_t tid = k_thread_create(&t2, t2_stack, STACK_SIZE, t2_fn,
				      NULL, NULL, NULL,
				      K_PRIO_COOP(1), 0, K_NO_WAIT);

	for (i = 0; i < 10; i++) {
		k_busy_wait(DELAY_US + (DELAY_US / 8));

		zassert_true(t2_count > i, "Another thread isn't"
			     " scheduled to run on other core");
	}
	k_thread_abort(tid);
}

static void smp_thread_entry(void *p1, void *p2, void *p3)
{
	int tnum = (int)p1;

	TC_PRINT("thread %d execution started\n", tnum);
	k_busy_wait(THREAD_DELAY);

	tdata[tnum].executed = 1;
	TC_PRINT("thread %d execution completed\n", tnum);
}


static void test_smp_single_core_coop_rr_threads(void)
{
	int i;

	k_sched_time_slice_set(100, 0);

	for (i = 0; i < THREADS_NUM; i++) {
		TC_PRINT(" thread %d created\n", i);
		tdata[i].executed = 0;
		tdata[i].tid = k_thread_create(&tthread[i], tstack[i],
					       STACK_SIZE, smp_thread_entry, (void *)i,
					       NULL, NULL,
					       K_PRIO_PREEMPT(K_LOWEST_THREAD_PRIO - 1),
					       0, 0);
	}

	k_busy_wait(THREAD_DELAY * THREADS_NUM + 10);

	/* Test point: All THREADS_NUM spawned threads must have
	 * executed in another core
	 */
	for (i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1, "Threads didn't get"
			     " scheduled on another core: %d", i);
	}

	for (i = 0; i < THREADS_NUM; i++) {
		k_thread_abort(tdata[i].tid);
		TC_PRINT(" thread %d aborted\n", i);
	}
}

static void test_smp_single_core_preemp_rr_threads(void)
{
	int i;

	k_thread_priority_set(k_current_get(),
			      K_PRIO_PREEMPT(K_HIGHEST_THREAD_PRIO));

	k_sched_time_slice_set(100, 0);

	for (i = 0; i < THREADS_NUM; i++) {
		TC_PRINT(" thread %d created\n", i);
		tdata[i].executed = 0;
		tdata[i].tid = k_thread_create(&tthread[i], tstack[i],
					       STACK_SIZE, smp_thread_entry, (void *)i,
					       NULL, NULL,
					       K_PRIO_PREEMPT(K_LOWEST_THREAD_PRIO - 1), 0, 0);
	}

	k_busy_wait(THREAD_DELAY * THREADS_NUM + 10);

	/* Test point: All THREADS_NUM spawned threads must have
	 *      * executed in another core
	 *           */
	for (i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1, "Threads didn't get"
			     " scheduled on another core: %d", i);
	}
}

static void test_smp_multi_core_coop_rr_threads(void)
{
	int i;

	k_sched_time_slice_set(100, 0);

	for (int i = 0; i < THREADS_NUM; i++) {
		TC_PRINT(" thread %d created\n", i);
		tdata[i].executed = 0;
		tdata[i].tid  = k_thread_create(&tthread[i], tstack[i],
						STACK_SIZE, smp_thread_entry, (void *)i,
						NULL, NULL,
						K_PRIO_PREEMPT(K_LOWEST_THREAD_PRIO - 1), 0, 0);
	}
	k_sleep((THREAD_DELAY * THREADS_NUM) / CONFIG_MP_NUM_CPUS);

	/* Test point: All THREADS_NUM spawned threads must have
	 * executed.
	 */
	for (i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1, "Threads didn't get"
			     "scheduled on all the cores");
	}
}

static void test_smp_multi_core_preemp_rr_threads(void)
{
	int i;

	k_thread_priority_set(k_current_get(), K_LOWEST_THREAD_PRIO);

	k_sched_time_slice_set(100, 0);

	for (int i = 0; i < THREADS_NUM; i++) {
		TC_PRINT(" thread %d created\n", i);
		tdata[i].executed = 0;
		tdata[i].tid = k_thread_create(&tthread[i], tstack[i],
					       STACK_SIZE, smp_thread_entry, (void *)i,
					       NULL, NULL,
					       K_PRIO_PREEMPT(K_LOWEST_THREAD_PRIO - 1), 0, 0);
	}

	/* Test point: All THREADS_NUM spawned threads must have
	 * executed.
	 */
	for (i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1, "Threads didn't get"
			     "scheduled on all the cores");
	}
}

static void test_smp_multi_core_yield_rr_threads(void)
{
	int i;

	k_thread_priority_set(k_current_get(),
			      K_PRIO_PREEMPT(K_LOWEST_THREAD_PRIO - 1));

	k_sched_time_slice_set(100, 0);

	for (int i = 0; i < THREADS_NUM; i++) {
		TC_PRINT(" thread %d created\n", i);
		tdata[i].executed = 0;
		tdata[i].tid  = k_thread_create(&tthread[i], tstack[i],
						STACK_SIZE, smp_thread_entry, (void *)i,
						NULL, NULL,
						K_PRIO_PREEMPT(K_LOWEST_THREAD_PRIO - 1), 0, 0);
	}
	k_yield();
	k_sleep((THREAD_DELAY * THREADS_NUM));

	/* Test point: All THREADS_NUM spawned threads must have
	 * executed.
	 */
	for (i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1, "Threads didn't get"
			     "scheduled on all the cores");
	}
}

void test_main(void)
{
	/* Sleep a bit to guarantee that both CPUs enter an idle
	 * thread from which they can exit correctly to run the main
	 * test.
	 */
	k_sleep(1000);

	/* Keep main thread's priority lower than spawned threads, so that
	 * it doesn't interrupt them in between
	 */
	k_thread_priority_set(k_current_get(), K_LOWEST_THREAD_PRIO);

	ztest_test_suite(smp,
			 ztest_unit_test(test_smp_preemp_threads),
			 ztest_unit_test(test_smp_coop_threads),
			 ztest_unit_test(test_smp_multi_core_coop_rr_threads),
			 ztest_unit_test(test_smp_multi_core_preemp_rr_threads),
			 ztest_unit_test(test_smp_multi_core_yield_rr_threads),
			 ztest_unit_test(test_smp_single_core_coop_rr_threads),
			 ztest_unit_test(test_smp_single_core_preemp_rr_threads)
			 );
	ztest_run_test_suite(smp);
}

/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>
#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>

#if CONFIG_MP_NUM_CPUS < 2
#error SMP test requires at least two CPUs!
#endif

#define T2_STACK_SIZE (2048 + CONFIG_TEST_EXTRA_STACKSIZE)
#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACKSIZE)
#define DELAY_US 50000
#define TIMEOUT 1000
#define EQUAL_PRIORITY 1
#define TIME_SLICE_MS 500
#define THREAD_DELAY 1

struct k_thread t2;
K_THREAD_STACK_DEFINE(t2_stack, T2_STACK_SIZE);

volatile int t2_count;
volatile int sync_count = -1;

K_SEM_DEFINE(cpuid_sema, 0, 1);
K_SEM_DEFINE(sema, 0, 1);

#define THREADS_NUM CONFIG_MP_NUM_CPUS

struct thread_info {
	k_tid_t tid;
	int executed;
	int priority;
	int cpu_id;
};
static volatile struct thread_info tinfo[THREADS_NUM];
static struct k_thread tthread[THREADS_NUM];
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREADS_NUM, STACK_SIZE);

static volatile int thread_started[THREADS_NUM - 1];

static int curr_cpu(void)
{
	unsigned int k = arch_irq_lock();
	int ret = arch_curr_cpu()->id;

	arch_irq_unlock(k);
	return ret;
}

/**
 * @brief Tests for SMP
 * @defgroup kernel_smp_tests SMP Tests
 * @ingroup all_tests
 * @{
 * @}
 */

static void t2_fn(void *a, void *b, void *c)
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
 * @brief Verify SMP with 2 cooperative threads
 *
 * @ingroup kernel_smp_tests
 *
 * @details Multi processing is verified by checking whether
 * 2 cooperative threads run simultaneously at different cores
 */
void test_smp_coop_threads(void)
{
	int i, ok = 1;

	k_tid_t tid = k_thread_create(&t2, t2_stack, T2_STACK_SIZE, t2_fn,
				      NULL, NULL, NULL,
				      K_PRIO_COOP(2), 0, K_NO_WAIT);

	/* Wait for the other thread (on a separate CPU) to actually
	 * start running.  We want synchrony to be as perfect as
	 * possible.
	 */
	t2_count = -1;
	while (t2_count == -1) {
	}

	for (i = 0; i < 10; i++) {
		/* Wait slightly longer than the other thread so our
		 * count will always be lower
		 */
		k_busy_wait(DELAY_US + (DELAY_US / 8));

		if (t2_count <= i) {
			ok = 0;
			break;
		}
	}

	k_thread_abort(tid);
	zassert_true(ok, "SMP test failed");
}

static void child_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	int parent_cpu_id = POINTER_TO_INT(p1);

	zassert_true(parent_cpu_id != curr_cpu(),
		     "Parent isn't on other core");

	sync_count++;
	k_sem_give(&cpuid_sema);
}

/**
 * @brief Verify CPU IDs of threads in SMP
 *
 * @ingroup kernel_smp_tests
 *
 * @details Verify whether thread running on other core is
 * parent thread from child thread
 */
void test_cpu_id_threads(void)
{
	/* Make sure idle thread runs on each core */
	k_sleep(K_MSEC(1000));

	int parent_cpu_id = curr_cpu();

	k_tid_t tid = k_thread_create(&t2, t2_stack, T2_STACK_SIZE, child_fn,
				      INT_TO_POINTER(parent_cpu_id), NULL,
				      NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);

	while (sync_count == -1) {
	}
	k_sem_take(&cpuid_sema, K_FOREVER);

	k_thread_abort(tid);
}

static void thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	int thread_num = POINTER_TO_INT(p1);
	int count = 0;

	tinfo[thread_num].executed  = 1;
	tinfo[thread_num].cpu_id = curr_cpu();

	while (count++ < 5) {
		k_busy_wait(DELAY_US);
	}
}

static void spin_for_threads_exit(void)
{
	for (int i = 0; i < THREADS_NUM - 1; i++) {
		volatile uint8_t *p = &tinfo[i].tid->base.thread_state;

		while (!(*p & _THREAD_DEAD)) {
		}
	}
	k_busy_wait(DELAY_US);
}

static void spawn_threads(int prio, int thread_num, int equal_prio,
			k_thread_entry_t thread_entry, int delay)
{
	int i;

	/* Spawn threads of priority higher than
	 * the previously created thread
	 */
	for (i = 0; i < thread_num; i++) {
		if (equal_prio) {
			tinfo[i].priority = prio;
		} else {
			/* Increase priority for each thread */
			tinfo[i].priority = prio - 1;
			prio = tinfo[i].priority;
		}
		tinfo[i].tid = k_thread_create(&tthread[i], tstack[i],
					       STACK_SIZE, thread_entry,
					       INT_TO_POINTER(i), NULL, NULL,
					       tinfo[i].priority, 0,
					       K_MSEC(delay));
		if (delay) {
			/* Increase delay for each thread */
			delay = delay + 10;
		}
	}
}

static void abort_threads(int num)
{
	for (int i = 0; i < num; i++) {
		k_thread_abort(tinfo[i].tid);
	}
}

static void cleanup_resources(void)
{
	for (int i = 0; i < THREADS_NUM; i++) {
		tinfo[i].tid = 0;
		tinfo[i].executed = 0;
		tinfo[i].priority = 0;
	}
}

/**
 * @brief Test cooperative threads non-preemption
 *
 * @ingroup kernel_smp_tests
 *
 * @details Spawn cooperative threads equal to number of cores
 * supported. Main thread will already be running on 1 core.
 * Check if the last thread created preempts any threads
 * already running.
 */
void test_coop_resched_threads(void)
{
	/* Spawn threads equal to number of cores,
	 * since we don't give up current CPU, last thread
	 * will not get scheduled
	 */
	spawn_threads(K_PRIO_COOP(10), THREADS_NUM, !EQUAL_PRIORITY,
		      &thread_entry, THREAD_DELAY);

	/* Wait for some time to let other core's thread run */
	k_busy_wait(DELAY_US);


	/* Reassure that cooperative thread's are not preempted
	 * by checking last thread's execution
	 * status. We know that all threads got rescheduled on
	 * other cores except the last one
	 */
	for (int i = 0; i < THREADS_NUM - 1; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "cooperative thread %d didn't run", i);
	}
	zassert_true(tinfo[THREADS_NUM - 1].executed == 0,
		     "cooperative thread is preempted");

	/* Abort threads created */
	abort_threads(THREADS_NUM);
	cleanup_resources();
}

/**
 * @brief Test preemptness of preemptive thread
 *
 * @ingroup kernel_smp_tests
 *
 * @details Create preemptive thread and let it run
 * on another core and verify if it gets preempted
 * if another thread of higher priority is spawned
 */
void test_preempt_resched_threads(void)
{
	/* Spawn threads  equal to number of cores,
	 * lower priority thread should
	 * be preempted by higher ones
	 */
	spawn_threads(K_PRIO_PREEMPT(10), THREADS_NUM, !EQUAL_PRIORITY,
		      &thread_entry, THREAD_DELAY);

	spin_for_threads_exit();

	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "preemptive thread %d didn't run", i);
	}

	/* Abort threads created */
	abort_threads(THREADS_NUM);
	cleanup_resources();
}

/**
 * @brief Validate behavior of thread when it yields
 *
 * @ingroup kernel_smp_tests
 *
 * @details Spawn cooperative threads equal to number
 * of cores, so last thread would be pending, call
 * yield() from main thread. Now, all threads must be
 * executed
 */
void test_yield_threads(void)
{
	/* Spawn threads equal to the number
	 * of cores, so the last thread would be
	 * pending.
	 */
	spawn_threads(K_PRIO_COOP(10), THREADS_NUM, !EQUAL_PRIORITY,
		      &thread_entry, !THREAD_DELAY);

	k_yield();
	k_busy_wait(DELAY_US);

	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "thread %d did not execute", i);

	}

	abort_threads(THREADS_NUM);
	cleanup_resources();
}

/**
 * @brief Test behavior of thread when it sleeps
 *
 * @ingroup kernel_smp_tests
 *
 * @details Spawn cooperative thread and call
 * sleep() from main thread. After timeout, all
 * threads has to be scheduled.
 */
void test_sleep_threads(void)
{
	spawn_threads(K_PRIO_COOP(10), THREADS_NUM, !EQUAL_PRIORITY,
		      &thread_entry, !THREAD_DELAY);

	k_msleep(TIMEOUT);

	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "thread %d did not execute", i);
	}

	abort_threads(THREADS_NUM);
	cleanup_resources();
}

static void thread_wakeup_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	int thread_num = POINTER_TO_INT(p1);

	thread_started[thread_num] = 1;

	k_msleep(DELAY_US * 1000);

	tinfo[thread_num].executed  = 1;
}

static void wakeup_on_start_thread(int tnum)
{
	int threads_started = 0, i;

	/* For each thread, spin waiting for it to first flag that
	 * it's going to sleep, and then that it's actually blocked
	 */
	for (i = 0; i < tnum; i++) {
		while (thread_started[i] == 0) {
		}
		while (!z_is_thread_prevented_from_running(tinfo[i].tid)) {
		}
	}

	for (i = 0; i < tnum; i++) {
		if (thread_started[i] == 1 && threads_started <= tnum) {
			threads_started++;
			k_wakeup(tinfo[i].tid);
		}
	}
	zassert_equal(threads_started, tnum,
		      "All threads haven't started");
}

static void check_wokeup_threads(int tnum)
{
	int threads_woke_up = 0, i;

	/* k_wakeup() isn't synchronous, give the other CPU time to
	 * schedule them
	 */
	k_busy_wait(200000);

	for (i = 0; i < tnum; i++) {
		if (tinfo[i].executed == 1 && threads_woke_up <= tnum) {
			threads_woke_up++;
		}
	}
	zassert_equal(threads_woke_up, tnum, "Threads did not wakeup");
}

/**
 * @brief Test behavior of wakeup() in SMP case
 *
 * @ingroup kernel_smp_tests
 *
 * @details Spawn number of threads equal to number of
 * remaining cores and let them sleep for a while. Call
 * wakeup() of those threads from parent thread and check
 * if they are all running
 */
void test_wakeup_threads(void)
{
	/* Spawn threads to run on all remaining cores */
	spawn_threads(K_PRIO_COOP(10), THREADS_NUM - 1, !EQUAL_PRIORITY,
		      &thread_wakeup_entry, !THREAD_DELAY);

	/* Check if all the threads have started, then call wakeup */
	wakeup_on_start_thread(THREADS_NUM - 1);

	/* Count threads which are woken up */
	check_wokeup_threads(THREADS_NUM - 1);

	/* Abort all threads and cleanup */
	abort_threads(THREADS_NUM - 1);
	cleanup_resources();
}

/* a thread for testing get current cpu */
static void thread_get_cpu_entry(void *p1, void *p2, void *p3)
{
	int bsp_id = *(int *)p1;
	int cpu_id = -1;

	/* get current cpu number for running thread */
	_cpu_t *curr_cpu = arch_curr_cpu();

	/**TESTPOINT: call arch_curr_cpu() to get cpu struct */
	zassert_true(curr_cpu != NULL,
			"test failed to get current cpu.");

	cpu_id = curr_cpu->id;

	zassert_true(bsp_id != cpu_id,
			"should not be the same with our BSP");

	/* loop forever to ensure running on this CPU */
	while (1) {
		k_busy_wait(DELAY_US);
	};
}

/**
 * @brief Test get a pointer of CPU
 *
 * @ingroup kernel_smp_tests
 *
 * @details Architecture layer provides a mechanism to return a pointer to the
 * current kernel CPU record of the running CPU.
 *
 * We call arch_curr_cpu() and get it's member, both in main and spwaned thread
 * speratively, and compare them. They shall be different in SMP enviornment.
 *
 * @see arch_curr_cpu()
 */
void test_get_cpu(void)
{
	k_tid_t thread_id;

	/* get current cpu number */
	int cpu_id = arch_curr_cpu()->id;

	thread_id = k_thread_create(&t2, t2_stack, T2_STACK_SIZE,
				      (k_thread_entry_t)thread_get_cpu_entry,
				      &cpu_id, NULL, NULL,
				      K_PRIO_COOP(2),
				      K_INHERIT_PERMS, K_NO_WAIT);

	k_busy_wait(DELAY_US);

	k_thread_abort(thread_id);
}

#ifdef CONFIG_TRACE_SCHED_IPI
/* global variable for testing send IPI */
static volatile int sched_ipi_has_called;

void z_trace_sched_ipi(void)
{
	sched_ipi_has_called++;
}

/**
 * @brief Test interprocessor interrupt
 *
 * @ingroup kernel_smp_tests
 *
 * @details Architecture layer provides a mechanism to issue an interprocessor
 * interrupt to all other CPUs in the system that calls the scheduler IPI
 * handler.
 *
 * We simply add a hook in z_sched_ipi(), in order to check if it has been
 * called once in another CPU except the caller, when arch_sched_ipi() is
 * called.
 *
 * @see arch_sched_ipi()
 */
void test_smp_ipi(void)
{
	TC_PRINT("cpu num=%d", CONFIG_MP_NUM_CPUS);

	for (int i = 0; i < 3 ; i++) {
		/* issue a sched ipi to tell other CPU to run thread */
		sched_ipi_has_called = 0;
		arch_sched_ipi();

		/* Need to wait longer than we think, loaded CI
		 * systems need to wait for host scheduling to run the
		 * other CPU's thread.
		 */
		k_msleep(100);

		/**TESTPOINT: check if enter our IPI interrupt handler */
		zassert_true(sched_ipi_has_called != 0,
				"did not receive IPI.(%d)",
				sched_ipi_has_called);
	}
}
#else
void test_smp_ipi(void)
{
	ztest_test_skip();
}
#endif

void test_main(void)
{
	/* Sleep a bit to guarantee that both CPUs enter an idle
	 * thread from which they can exit correctly to run the main
	 * test.
	 */
	k_sleep(K_MSEC(10));

	ztest_test_suite(smp,
			 ztest_unit_test(test_smp_coop_threads),
			 ztest_unit_test(test_cpu_id_threads),
			 ztest_unit_test(test_coop_resched_threads),
			 ztest_unit_test(test_preempt_resched_threads),
			 ztest_unit_test(test_yield_threads),
			 ztest_unit_test(test_sleep_threads),
			 ztest_unit_test(test_wakeup_threads),
			 ztest_unit_test(test_smp_ipi),
			 ztest_unit_test(test_get_cpu)
			 );
	ztest_run_test_suite(smp);
}

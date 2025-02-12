/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/kernel_structs.h>

#if CONFIG_MP_MAX_NUM_CPUS < 2
#error SMP test requires at least two CPUs!
#endif

#define RUN_FACTOR (CONFIG_SMP_TEST_RUN_FACTOR / 100.0)

#define T2_STACK_SIZE (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define DELAY_US 50000
#define TIMEOUT 1000
#define EQUAL_PRIORITY 1
#define TIME_SLICE_MS 500
#define THREAD_DELAY 1
#define SLEEP_MS_LONG ((int)(15000 * RUN_FACTOR))

struct k_thread t2;
K_THREAD_STACK_DEFINE(t2_stack, T2_STACK_SIZE);

volatile int t2_count;
volatile int sync_count = -1;

static int main_thread_id;
static int child_thread_id;
volatile int rv;

K_SEM_DEFINE(cpuid_sema, 0, 1);
K_SEM_DEFINE(sema, 0, 1);
static struct k_mutex smutex;
static struct k_sem smp_sem;

#define MAX_NUM_THREADS CONFIG_MP_MAX_NUM_CPUS

struct thread_info {
	k_tid_t tid;
	int executed;
	int priority;
	int cpu_id;
};
static ZTEST_BMEM volatile struct thread_info tinfo[MAX_NUM_THREADS];
static struct k_thread tthread[MAX_NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(tstack, MAX_NUM_THREADS, STACK_SIZE);

static volatile int thread_started[MAX_NUM_THREADS - 1];

static struct k_poll_signal tsignal[MAX_NUM_THREADS];
static struct k_poll_event tevent[MAX_NUM_THREADS];

static int curr_cpu(void)
{
	unsigned int k = arch_irq_lock();
	int ret = arch_curr_cpu()->id;

	arch_irq_unlock(k);
	return ret;
}

/**
 * @brief SMP
 * @defgroup kernel_smp_tests SMP Tests
 * @ingroup all_tests
 * @{
 * @}
 */

/**
 * @defgroup kernel_smp_integration_tests SMP Integration Tests
 * @ingroup kernel_smp_tests
 * @{
 * @}
 */

/**
 * @defgroup kernel_smp_module_tests SMP Module Tests
 * @ingroup kernel_smp_tests
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
ZTEST(smp, test_smp_coop_threads)
{
	int i, ok = 1;

	if (!IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) {
		/* The spawned thread enters an infinite loop, so it can't be
		 * successfully aborted via an IPI.  Just skip in that
		 * configuration.
		 */
		ztest_test_skip();
	}

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
	k_thread_join(tid, K_FOREVER);
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
ZTEST(smp, test_cpu_id_threads)
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
	k_thread_join(tid, K_FOREVER);
}

static void thread_entry_fn(void *p1, void *p2, void *p3)
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
	unsigned int num_threads = arch_num_cpus();

	for (int i = 0; i < num_threads - 1; i++) {
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

	for (int i = 0; i < num; i++) {
		k_thread_join(tinfo[i].tid, K_FOREVER);
	}
}

static void cleanup_resources(void)
{
	unsigned int num_threads = arch_num_cpus();

	for (int i = 0; i < num_threads; i++) {
		tinfo[i].tid = 0;
		tinfo[i].executed = 0;
		tinfo[i].priority = 0;
	}
}

static void __no_optimization thread_ab_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
	}
}

#define SPAWN_AB_PRIO K_PRIO_COOP(10)

/**
 * @brief Verify the code path when we do context switch in k_thread_abort on SMP system
 *
 * @ingroup kernel_smp_tests
 *
 * @details test logic:
 * - The ztest thread has cooperative priority.
 * - From ztest thread we spawn N number of cooperative threads, where N = number of CPUs.
 *   - The spawned cooperative are executing infinite loop (so they occupy CPU core until they are
 *     aborted).
 *   - We have (number of CPUs - 1) spawned threads run and executing infinite loop, as current CPU
 *     is occupied by ztest cooperative thread. Due to that the last of spawned threads is ready but
 *     not executing.
 * - We abort spawned threads one-by-one from the ztest thread.
 *   - At the first k_thread_abort call the ztest thread will be preempted by the remaining spawned
 *     thread which has higher priority than ztest thread.
 *     But... k_thread_abort call should has destroyed one of the spawned threads, so ztest thread
 *     should have a CPU available to run on.
 * - We expect that all spawned threads will be aborted successfully.
 *
 * This was the test case for zephyrproject-rtos/zephyr#58040 issue where this test caused system
 * hang.
 */

ZTEST(smp, test_coop_switch_in_abort)
{
	k_tid_t tid[MAX_NUM_THREADS];
	unsigned int num_threads = arch_num_cpus();
	unsigned int i;

	zassert_true(_current->base.prio < 0, "test case relies on ztest thread be cooperative");
	zassert_true(_current->base.prio > SPAWN_AB_PRIO,
		     "spawn test need to have higher priority than ztest thread");

	/* Spawn N number of cooperative threads, where N = number of CPUs */
	for (i = 0; i < num_threads; i++) {
		tid[i] = k_thread_create(&tthread[i], tstack[i],
					 STACK_SIZE, thread_ab_entry,
					 NULL, NULL, NULL,
					 SPAWN_AB_PRIO, 0, K_NO_WAIT);
	}

	/* Wait for some time to let spawned threads on other cores run and start executing infinite
	 * loop.
	 */
	k_busy_wait(DELAY_US * 4);

	/* At this time we have (number of CPUs - 1) spawned threads run and executing infinite loop
	 * on other CPU cores, as current CPU is occupied by this ztest cooperative thread.
	 * Due to that the last of spawned threads is ready but not executing.
	 */

	/* Abort all spawned threads one-by-one. At the first k_thread_abort call the context
	 * switch will happen and the last 'spawned' thread will start.
	 * We should successfully abort all threads.
	 */
	for (i = 0; i < num_threads; i++) {
		k_thread_abort(tid[i]);
	}

	/* Cleanup */
	for (i = 0; i < num_threads; i++) {
		zassert_equal(k_thread_join(tid[i], K_FOREVER), 0);
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
ZTEST(smp, test_coop_resched_threads)
{
	unsigned int num_threads = arch_num_cpus();

	/* Spawn threads equal to number of cores,
	 * since we don't give up current CPU, last thread
	 * will not get scheduled
	 */
	spawn_threads(K_PRIO_COOP(10), num_threads, !EQUAL_PRIORITY,
		      &thread_entry_fn, THREAD_DELAY);

	/* Wait for some time to let other core's thread run */
	k_busy_wait(DELAY_US);


	/* Reassure that cooperative thread's are not preempted
	 * by checking last thread's execution
	 * status. We know that all threads got rescheduled on
	 * other cores except the last one
	 */
	for (int i = 0; i < num_threads - 1; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "cooperative thread %d didn't run", i);
	}
	zassert_true(tinfo[num_threads - 1].executed == 0,
		     "cooperative thread is preempted");

	/* Abort threads created */
	abort_threads(num_threads);
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
ZTEST(smp, test_preempt_resched_threads)
{
	unsigned int num_threads = arch_num_cpus();

	/* Spawn threads  equal to number of cores,
	 * lower priority thread should
	 * be preempted by higher ones
	 */
	spawn_threads(K_PRIO_PREEMPT(10), num_threads, !EQUAL_PRIORITY,
		      &thread_entry_fn, THREAD_DELAY);

	spin_for_threads_exit();

	for (int i = 0; i < num_threads; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "preemptive thread %d didn't run", i);
	}

	/* Abort threads created */
	abort_threads(num_threads);
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
ZTEST(smp, test_yield_threads)
{
	unsigned int num_threads = arch_num_cpus();

	/* Spawn threads equal to the number
	 * of cores, so the last thread would be
	 * pending.
	 */
	spawn_threads(K_PRIO_COOP(10), num_threads, !EQUAL_PRIORITY,
		      &thread_entry_fn, !THREAD_DELAY);

	k_yield();
	k_busy_wait(DELAY_US);

	for (int i = 0; i < num_threads; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "thread %d did not execute", i);

	}

	abort_threads(num_threads);
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
ZTEST(smp, test_sleep_threads)
{
	unsigned int num_threads = arch_num_cpus();

	spawn_threads(K_PRIO_COOP(10), num_threads, !EQUAL_PRIORITY,
		      &thread_entry_fn, !THREAD_DELAY);

	k_msleep(TIMEOUT);

	for (int i = 0; i < num_threads; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "thread %d did not execute", i);
	}

	abort_threads(num_threads);
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
ZTEST(smp, test_wakeup_threads)
{
	unsigned int num_threads = arch_num_cpus();

	/* Spawn threads to run on all remaining cores */
	spawn_threads(K_PRIO_COOP(10), num_threads - 1, !EQUAL_PRIORITY,
		      &thread_wakeup_entry, !THREAD_DELAY);

	/* Check if all the threads have started, then call wakeup */
	wakeup_on_start_thread(num_threads - 1);

	/* Count threads which are woken up */
	check_wokeup_threads(num_threads - 1);

	/* Abort all threads and cleanup */
	abort_threads(num_threads - 1);
	cleanup_resources();
}

/* a thread for testing get current cpu */
static void thread_get_cpu_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

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
	}
}

/**
 * @brief Test get a pointer of CPU
 *
 * @ingroup kernel_smp_module_tests
 *
 * @details
 * Test Objective:
 * - To verify architecture layer provides a mechanism to return a pointer to the
 *   current kernel CPU record of the running CPU.
 *   We call arch_curr_cpu() and get its member, both in main and spawned thread
 *   separately, and compare them. They shall be different in SMP environment.
 *
 * Testing techniques:
 * - Interface testing, function and block box testing,
 *   dynamic analysis and testing,
 *
 * Prerequisite Conditions:
 * - CONFIG_SMP=y, and the HW platform must support SMP.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# In main thread, call arch_curr_cpu() to get it's member "id",then store it
 *  into a variable thread_id.
 * -# Spawn a thread t2, and pass the stored thread_id to it, then call
 *  k_busy_wait() 50us to wait for thread run and won't be swapped out.
 * -# In thread t2, call arch_curr_cpu() to get pointer of current cpu data. Then
 *  check if it not NULL.
 * -# Store the member id via accessing pointer of current cpu data to var cpu_id.
 * -# Check if cpu_id is not equaled to bsp_id that we pass into thread.
 * -# Call k_busy_wait() and loop forever.
 * -# In main thread, terminate the thread t2 before exit.
 *
 * Expected Test Result:
 * - The pointer of current cpu data that we got from function call is correct.
 *
 * Pass/Fail Criteria:
 * - Successful if the check of step 3,5 are all passed.
 * - Failure if one of the check of step 3,5 is failed.
 *
 * Assumptions and Constraints:
 * - This test using for the platform that support SMP, in our current scenario
 *   , only x86_64, arc and xtensa supported.
 *
 * @see arch_curr_cpu()
 */
static int _cpu_id;
ZTEST(smp, test_get_cpu)
{
	k_tid_t thread_id;

	if (!IS_ENABLED(CONFIG_SCHED_IPI_SUPPORTED)) {
		/* The spawned thread enters an infinite loop, so it can't be
		 * successfully aborted via an IPI.  Just skip in that
		 * configuration.
		 */
		ztest_test_skip();
	}

	/* get current cpu number */
	_cpu_id = arch_curr_cpu()->id;

	thread_id = k_thread_create(&t2, t2_stack, T2_STACK_SIZE,
				      thread_get_cpu_entry,
				      &_cpu_id, NULL, NULL,
				      K_PRIO_COOP(2),
				      K_INHERIT_PERMS, K_NO_WAIT);

	k_busy_wait(DELAY_US);

	k_thread_abort(thread_id);
	k_thread_join(thread_id, K_FOREVER);
}

#ifdef CONFIG_TRACE_SCHED_IPI
/* global variable for testing send IPI */
static volatile int sched_ipi_has_called;

void z_trace_sched_ipi(void)
{
	sched_ipi_has_called++;
}
#endif

/**
 * @brief Test interprocessor interrupt
 *
 * @ingroup kernel_smp_integration_tests
 *
 * @details
 * Test Objective:
 * - To verify architecture layer provides a mechanism to issue an interprocessor
 *   interrupt to all other CPUs in the system that calls the scheduler IPI.
 *   We simply add a hook in z_sched_ipi(), in order to check if it has been
 *   called once in another CPU except the caller, when arch_sched_broadcast_ipi()
 *   is called.
 *
 * Testing techniques:
 * - Interface testing, function and block box testing,
 *   dynamic analysis and testing
 *
 * Prerequisite Conditions:
 * - CONFIG_SMP=y, and the HW platform must support SMP.
 * - CONFIG_TRACE_SCHED_IPI=y was set.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# In main thread, given a global variable sched_ipi_has_called equaled zero.
 * -# Call arch_sched_broadcast_ipi() then sleep for 100ms.
 * -# In z_sched_ipi() handler, increment the sched_ipi_has_called.
 * -# In main thread, check the sched_ipi_has_called is not equaled to zero.
 * -# Repeat step 1 to 4 for 3 times.
 *
 * Expected Test Result:
 * - The pointer of current cpu data that we got from function call is correct.
 *
 * Pass/Fail Criteria:
 * - Successful if the check of step 4 are all passed.
 * - Failure if one of the check of step 4 is failed.
 *
 * Assumptions and Constraints:
 * - This test using for the platform that support SMP, in our current scenario
 *   , only x86_64 and arc supported.
 *
 * @see arch_sched_broadcast_ipi()
 */
#ifdef CONFIG_SCHED_IPI_SUPPORTED
ZTEST(smp, test_smp_ipi)
{
#ifndef CONFIG_TRACE_SCHED_IPI
	ztest_test_skip();
#endif

	TC_PRINT("cpu num=%d", arch_num_cpus());

	for (int i = 0; i < 3 ; i++) {
		/* issue a sched ipi to tell other CPU to run thread */
		sched_ipi_has_called = 0;
		arch_sched_broadcast_ipi();

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
#endif

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	static int trigger;

	if (reason != K_ERR_KERNEL_OOPS) {
		printk("wrong error reason\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}

	if (trigger == 0) {
		child_thread_id = curr_cpu();
		trigger++;
	} else {
		main_thread_id = curr_cpu();

		/* Verify the fatal was happened on different core */
		zassert_true(main_thread_id != child_thread_id,
					"fatal on the same core");
	}
}

void entry_oops(void *p1, void *p2, void *p3)
{
	k_oops();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
}

/**
 * @brief Test fatal error can be triggered on different core

 * @details When CONFIG_SMP is enabled, on some multiprocessor
 * platforms, exception can be triggered on different core at
 * the same time.
 *
 * @ingroup kernel_common_tests
 */
ZTEST(smp, test_fatal_on_smp)
{
	/* Creat a child thread and trigger a crash */
	k_thread_create(&t2, t2_stack, T2_STACK_SIZE, entry_oops,
				      NULL, NULL, NULL,
				      K_PRIO_PREEMPT(2), 0, K_NO_WAIT);

	/* hold cpu and wait for thread trigger exception and being terminated */
	k_busy_wait(5 * DELAY_US);

	/* Verify that child thread is no longer running. We can't simply use k_thread_join here
	 * as we don't want to introduce reschedule point here.
	 */
	zassert_true(z_is_thread_state_set(&t2, _THREAD_DEAD));

	/* Manually trigger the crash in mainthread */
	entry_oops(NULL, NULL, NULL);

	/* should not be here */
	ztest_test_fail();
}

static void workq_handler(struct k_work *work)
{
	child_thread_id = curr_cpu();
}

/**
 * @brief Test system workq run on different core

 * @details When macro CONFIG_SMP is enabled, workq can be run
 * on different core.
 *
 * @ingroup kernel_common_tests
 */
ZTEST(smp, test_workq_on_smp)
{
	static struct k_work work;

	k_work_init(&work, workq_handler);

	/* submit work item on system workq */
	k_work_submit(&work);

	/* Wait for some time to let other core's thread run */
	k_busy_wait(DELAY_US);

	/* check work have finished */
	zassert_equal(k_work_busy_get(&work), 0);

	main_thread_id = curr_cpu();

	/* Verify the ztest thread and system workq run on different core */
	zassert_true(main_thread_id != child_thread_id,
		"system workq run on the same core");
}

static void t1_mutex_lock(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* t1 will get mutex first */
	k_mutex_lock((struct k_mutex *)p1, K_FOREVER);

	k_msleep(2);

	k_mutex_unlock((struct k_mutex *)p1);
}

static void t2_mutex_lock(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_equal(_current->base.global_lock_count, 0,
			"thread global lock cnt %d is incorrect",
			_current->base.global_lock_count);

	k_mutex_lock((struct k_mutex *)p1, K_FOREVER);

	zassert_equal(_current->base.global_lock_count, 0,
			"thread global lock cnt %d is incorrect",
			_current->base.global_lock_count);

	k_mutex_unlock((struct k_mutex *)p1);

	/**TESTPOINT: z_smp_release_global_lock() has been call during
	 * context switch but global_lock_cnt has not been decrease
	 * because no irq_lock() was called.
	 */
	zassert_equal(_current->base.global_lock_count, 0,
			"thread global lock cnt %d is incorrect",
			_current->base.global_lock_count);
}

/**
 * @brief Test scenario that a thread release the global lock
 *
 * @ingroup kernel_smp_tests
 *
 * @details Validate the scenario that make the internal APIs of SMP
 * z_smp_release_global_lock() to be called.
 */
ZTEST(smp, test_smp_release_global_lock)
{
	k_mutex_init(&smutex);

	tinfo[0].tid =
	k_thread_create(&tthread[0], tstack[0], STACK_SIZE,
			t1_mutex_lock,
			&smutex, NULL, NULL,
			K_PRIO_PREEMPT(5),
			K_INHERIT_PERMS, K_NO_WAIT);

	tinfo[1].tid =
	k_thread_create(&tthread[1], tstack[1], STACK_SIZE,
		t2_mutex_lock,
			&smutex, NULL, NULL,
			K_PRIO_PREEMPT(3),
			K_INHERIT_PERMS, K_MSEC(1));

	/* Hold one of the cpu to ensure context switch as we wanted
	 * can happen in another cpu.
	 */
	k_busy_wait(20000);

	k_thread_join(tinfo[1].tid, K_FOREVER);
	k_thread_join(tinfo[0].tid, K_FOREVER);
	cleanup_resources();
}

#define LOOP_COUNT ((int)(20000 * RUN_FACTOR))

enum sync_t {
	LOCK_IRQ,
	LOCK_SEM,
	LOCK_MUTEX
};

static int global_cnt;
static struct k_mutex smp_mutex;

static void (*sync_lock)(void *);
static void (*sync_unlock)(void *);

static void sync_lock_dummy(void *k)
{
	/* no sync lock used */
}

static void sync_lock_irq(void *k)
{
	*((unsigned int *)k) = irq_lock();
}

static void sync_unlock_irq(void *k)
{
	irq_unlock(*(unsigned int *)k);
}

static void sync_lock_sem(void *k)
{
	k_sem_take(&smp_sem, K_FOREVER);
}

static void sync_unlock_sem(void *k)
{
	k_sem_give(&smp_sem);
}

static void sync_lock_mutex(void *k)
{
	k_mutex_lock(&smp_mutex, K_FOREVER);
}

static void sync_unlock_mutex(void *k)
{
	k_mutex_unlock(&smp_mutex);
}

static void sync_init(int lock_type)
{
	switch (lock_type) {
	case LOCK_IRQ:
		sync_lock = sync_lock_irq;
		sync_unlock = sync_unlock_irq;
		break;
	case LOCK_SEM:
		sync_lock = sync_lock_sem;
		sync_unlock = sync_unlock_sem;
		k_sem_init(&smp_sem, 1, 3);
		break;
	case LOCK_MUTEX:
		sync_lock = sync_lock_mutex;
		sync_unlock = sync_unlock_mutex;
		k_mutex_init(&smp_mutex);
		break;

	default:
		sync_lock = sync_unlock = sync_lock_dummy;
	}
}

static void inc_global_cnt(void *a, void *b, void *c)
{
	int key;

	for (int i = 0; i < LOOP_COUNT; i++) {

		sync_lock(&key);

		global_cnt++;
		global_cnt--;
		global_cnt++;

		sync_unlock(&key);
	}
}

static int run_concurrency(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	int type = POINTER_TO_INT(p1);
	k_thread_entry_t func = p2;
	uint32_t start_t, end_t;

	sync_init(type);
	global_cnt = 0;
	start_t = k_cycle_get_32();

	tinfo[0].tid =
	k_thread_create(&tthread[0], tstack[0], STACK_SIZE,
			func,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1),
			K_INHERIT_PERMS, K_NO_WAIT);

	tinfo[1].tid =
	k_thread_create(&tthread[1], tstack[1], STACK_SIZE,
			func,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1),
			K_INHERIT_PERMS, K_NO_WAIT);

	k_tid_t tid =
	k_thread_create(&t2, t2_stack, T2_STACK_SIZE,
			func,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1),
			K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tinfo[0].tid, K_FOREVER);
	k_thread_join(tinfo[1].tid, K_FOREVER);
	k_thread_join(tid, K_FOREVER);
	cleanup_resources();

	end_t =  k_cycle_get_32();

	printk("type %d: cnt %d, spend %u ms\n", type, global_cnt,
		k_cyc_to_ms_ceil32(end_t - start_t));

	return global_cnt == (LOOP_COUNT * 3);
}

/**
 * @brief Test if the concurrency of SMP works or not
 *
 * @ingroup kernel_smp_tests
 *
 * @details Validate the global lock and unlock API of SMP are thread-safe.
 * We make 3 thread to increase the global count in different cpu and
 * they both do locking then unlocking for LOOP_COUNT times. It shall be no
 * deadlock happened and total global count shall be 3 * LOOP COUNT.
 *
 * We show the 4 kinds of scenario:
 * - No any lock used
 * - Use global irq lock
 * - Use semaphore
 * - Use mutex
 */
ZTEST(smp, test_inc_concurrency)
{
	/* increasing global var with irq lock */
	zassert_true(run_concurrency(INT_TO_POINTER(LOCK_IRQ), inc_global_cnt, NULL),
			"total count %d is wrong(i)", global_cnt);

	/* increasing global var with irq lock */
	zassert_true(run_concurrency(INT_TO_POINTER(LOCK_SEM), inc_global_cnt, NULL),
			"total count %d is wrong(s)", global_cnt);

	/* increasing global var with irq lock */
	zassert_true(run_concurrency(INT_TO_POINTER(LOCK_MUTEX), inc_global_cnt, NULL),
			"total count %d is wrong(M)", global_cnt);
}

/**
 * @brief Torture test for context switching code
 *
 * @ingroup kernel_smp_tests
 *
 * @details Leverage the polling API to stress test the context switching code.
 *          This test will hammer all the CPUs with thread swapping requests.
 */
static void process_events(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	uintptr_t id = (uintptr_t) arg0;

	while (1) {
		k_poll(&tevent[id], 1, K_FOREVER);

		if (tevent[id].signal->result != 0x55) {
			ztest_test_fail();
		}

		tevent[id].signal->signaled = 0;
		tevent[id].state = K_POLL_STATE_NOT_READY;

		k_poll_signal_reset(&tsignal[id]);
	}
}

static void signal_raise(void *arg0, void *arg1, void *arg2)
{
	unsigned int num_threads = arch_num_cpus();

	while (1) {
		for (uintptr_t i = 0; i < num_threads; i++) {
			k_poll_signal_raise(&tsignal[i], 0x55);
		}
	}
}

ZTEST(smp, test_smp_switch_torture)
{
	unsigned int num_threads = arch_num_cpus();

	if (CONFIG_SMP_TEST_RUN_FACTOR == 0) {
		/* If CONFIG_SMP_TEST_RUN_FACTOR is zero,
		 * the switch torture test is effectively
		 * not doing anything as the k_sleep()
		 * below is not going to sleep at all,
		 * and all created threads are being
		 * terminated (almost) immediately after
		 * creation. So if run factor is zero,
		 * mark the test as skipped.
		 */
		ztest_test_skip();
	}

	for (uintptr_t i = 0; i < num_threads; i++) {
		k_poll_signal_init(&tsignal[i]);
		k_poll_event_init(&tevent[i], K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY, &tsignal[i]);

		k_thread_create(&tthread[i], tstack[i], STACK_SIZE,
				process_events,
				(void *) i, NULL, NULL, K_PRIO_PREEMPT(i + 1),
				K_INHERIT_PERMS, K_NO_WAIT);
	}

	k_thread_create(&t2, t2_stack, T2_STACK_SIZE, signal_raise,
			NULL, NULL, NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);

	k_sleep(K_MSEC(SLEEP_MS_LONG));

	k_thread_abort(&t2);
	k_thread_join(&t2, K_FOREVER);
	for (uintptr_t i = 0; i < num_threads; i++) {
		k_thread_abort(&tthread[i]);
		k_thread_join(&tthread[i], K_FOREVER);
	}
}

/**
 * @brief Torture test for cpu affinity code
 *
 * @ingroup kernel_smp_tests
 *
 * @details Pin thread to a specific cpu. Once thread gets cpu, check
 *          the cpu id is correct and then thread will give up cpu.
 */
#ifdef CONFIG_SCHED_CPU_MASK
static void check_affinity(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	int affinity = POINTER_TO_INT(arg0);
	int counter = 30;

	while (counter != 0) {
		zassert_equal(affinity, curr_cpu(), "Affinity test failed.");
		counter--;
		k_yield();
	}
}

ZTEST(smp, test_smp_affinity)
{
	int num_threads = arch_num_cpus();

	for (int i = 0; i < num_threads; ++i) {
		k_thread_create(&tthread[i], tstack[i],
					       STACK_SIZE, check_affinity,
					       INT_TO_POINTER(i), NULL, NULL,
					       0, 0, K_FOREVER);

		k_thread_cpu_pin(&tthread[i], i);
		k_thread_start(&tthread[i]);
	}

	for (int i = 0; i < num_threads; i++) {
		k_thread_join(&tthread[i], K_FOREVER);
	}
}
#endif

static void *smp_tests_setup(void)
{
	/* Sleep a bit to guarantee that both CPUs enter an idle
	 * thread from which they can exit correctly to run the main
	 * test.
	 */
	k_sleep(K_MSEC(10));

	return NULL;
}

ZTEST_SUITE(smp, NULL, smp_tests_setup, NULL, NULL, NULL);

/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <ksched.h>

#if CONFIG_MP_MAX_NUM_CPUS < 2
#error SMP test requires at least two CPUs!
#endif

/*
 * The test is designed to work with no more than 12 CPUs.
 * If attempting to run on a platform with more than that,
 * create a custom board overlay file to reduce the number
 * of CPUs to 12.
 */
#if CONFIG_MP_MAX_NUM_CPUS > 12
#error "Test only supports up to 12 CPUs\nReduce CONFIG_MP_MAX_NUM_CPUS\n"
#endif

#define RUN_FACTOR (CONFIG_SMP_TEST_RUN_FACTOR / 100.0)

#define T2_STACK_SIZE (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define DELAY_US 50000
#define TIMEOUT 5000
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
 * @brief Symmetric multiprocessing (SMP) tests
 *
 * @defgroup kernel_smp_tests SMP Tests
 *
 * @ingroup all_tests
 *
 * These tests validate that the kernel schedules threads across all available
 * CPUs, that per-CPU state is reported correctly, and that the SMP-specific
 * paths of the scheduler (IPIs, the global lock, context switching) behave as
 * documented.
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
 * @brief Verify that two cooperative threads execute simultaneously on
 *        different CPUs.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * A cooperative thread cannot be preempted, so on a uniprocessor system a
 * spinning cooperative thread would starve every other thread. The test
 * spawns such a thread and checks from the (also cooperative) test thread
 * that it keeps making progress, which is only possible if the two threads
 * are running on separate CPUs at the same time.
 *
 * Test steps:
 * - Spawn a cooperative thread that busy-waits and increments a counter.
 * - Wait until the spawned thread has started running.
 * - Ten times, busy-wait slightly longer than the spawned thread's iteration
 *   and check that its counter has advanced past the local iteration count.
 * - Abort and join the spawned thread.
 *
 * Expected result:
 * - The spawned thread's counter advances on every iteration, proving both
 *   cooperative threads run concurrently.
 *
 * @see k_thread_create()
 * @see K_PRIO_COOP()
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
 * @brief Verify that a child thread is scheduled on a different CPU than its
 *        parent.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * Once the secondary CPUs have been brought up, a newly created thread is
 * eligible to run on any of them. The child thread reads its own CPU id and
 * compares it against the id its parent recorded before the create call, so a
 * pass proves the scheduler placed the two threads on distinct CPUs.
 *
 * Test steps:
 * - Sleep briefly so every CPU has entered its idle thread.
 * - Record the test thread's CPU id via arch_curr_cpu().
 * - Create a preemptible child thread and pass it the recorded id.
 * - In the child, read the current CPU id and assert it differs from the
 *   parent's, then signal a semaphore.
 * - Take the semaphore, then abort and join the child.
 *
 * Expected result:
 * - The child observes a CPU id different from its parent's.
 *
 * @see arch_curr_cpu()
 * @see k_thread_create()
 */
ZTEST(smp, test_smp_cpu_id_threads)
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
 *
 * Test steps:
 * - Spawn one cooperative thread per CPU, each spinning in an infinite loop.
 * - Busy-wait so the spawned threads occupy every other CPU.
 * - Abort the spawned threads one by one from the test thread.
 * - Join every aborted thread.
 *
 * Expected result:
 * - All spawned threads are aborted and joined; the system does not hang.
 *
 * @see k_thread_abort()
 * @see k_thread_join()
 */
ZTEST(smp, test_smp_coop_switch_in_abort)
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
 * @brief Verify that a cooperative thread is never preempted on an SMP system.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * The test thread itself occupies one CPU, so spawning one cooperative thread
 * per CPU leaves the last one ready but with no CPU to run on. Because
 * cooperative threads cannot be preempted, that last thread must stay
 * unscheduled even though it has the highest priority of the group.
 *
 * Test steps:
 * - Spawn one cooperative thread per CPU, each with a higher priority than the
 *   previous one.
 * - Busy-wait to give the threads placed on other CPUs time to run.
 * - Check the per-thread "executed" flag of every spawned thread.
 * - Abort the spawned threads and clean up.
 *
 * Expected result:
 * - All threads but the last one execute.
 * - The last (highest priority) thread does not execute, as no running
 *   cooperative thread yields its CPU to it.
 *
 * @see K_PRIO_COOP()
 * @see k_thread_create()
 */
ZTEST(smp, test_smp_coop_resched_threads)
{
	unsigned int num_threads = arch_num_cpus();

	/* Spawn threads equal to number of cores,
	 * since we don't give up current CPU, last thread
	 * will not get scheduled
	 */
	spawn_threads(K_PRIO_COOP(12), num_threads, !EQUAL_PRIORITY,
		      &thread_entry_fn, THREAD_DELAY);

	/* Wait for some time to let other core's thread run */
	k_busy_wait(DELAY_US * 5);


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
 * @brief Verify that preemptible threads are rescheduled across all CPUs.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * Unlike cooperative threads, preemptible threads yield their CPU to any
 * higher-priority thread that becomes ready. Spawning one preemptible thread
 * per CPU with increasing priority must therefore let every one of them run to
 * completion, including the last thread created, which preempts an already
 * running one.
 *
 * Test steps:
 * - Spawn one preemptible thread per CPU, each with a higher priority than the
 *   previous one.
 * - Spin until all spawned threads have terminated.
 * - Check the per-thread "executed" flag of every spawned thread.
 * - Abort the spawned threads and clean up.
 *
 * Expected result:
 * - Every spawned preemptible thread executes.
 *
 * @see K_PRIO_PREEMPT()
 * @see k_thread_create()
 */
ZTEST(smp, test_smp_preempt_resched_threads)
{
	unsigned int num_threads = arch_num_cpus();

	/* Spawn threads  equal to number of cores,
	 * lower priority thread should
	 * be preempted by higher ones
	 */
	spawn_threads(K_PRIO_PREEMPT(12), num_threads, !EQUAL_PRIORITY,
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
 * @brief Verify that k_yield() releases a CPU to a pending cooperative thread.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * With one cooperative thread spawned per CPU, the last one stays ready but
 * unscheduled because the test thread occupies the remaining CPU. Yielding
 * from the test thread hands that CPU over, so the pending thread must then
 * be scheduled and run to completion.
 *
 * Test steps:
 * - Spawn one cooperative thread per CPU with no start delay.
 * - Call k_yield() from the test thread, then busy-wait.
 * - Check the per-thread "executed" flag of every spawned thread.
 * - Abort the spawned threads and clean up.
 *
 * Expected result:
 * - Every spawned thread executes, including the one that was pending before
 *   the yield.
 *
 * @see k_yield()
 */
ZTEST(smp, test_smp_yield_threads)
{
	unsigned int num_threads = arch_num_cpus();

	/* Spawn threads equal to the number
	 * of cores, so the last thread would be
	 * pending.
	 */
	spawn_threads(K_PRIO_COOP(12), num_threads, !EQUAL_PRIORITY,
		      &thread_entry_fn, !THREAD_DELAY);

	k_yield();
	k_busy_wait(DELAY_US * 5);

	for (int i = 0; i < num_threads; i++) {
		zassert_true(tinfo[i].executed == 1,
			     "thread %d did not execute", i);

	}

	abort_threads(num_threads);
	cleanup_resources();
}

/**
 * @brief Verify that sleeping releases a CPU to a pending cooperative thread.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * Same scenario as the yield case, but the test thread gives up its CPU by
 * sleeping instead of yielding. While it sleeps, the thread that had no CPU
 * available must be scheduled and run.
 *
 * Test steps:
 * - Spawn one cooperative thread per CPU with no start delay.
 * - Call k_msleep() from the test thread for longer than the threads need.
 * - Check the per-thread "executed" flag of every spawned thread.
 * - Abort the spawned threads and clean up.
 *
 * Expected result:
 * - Every spawned thread has executed by the time the sleep expires.
 *
 * @see k_msleep()
 */
ZTEST(smp, test_smp_sleep_threads)
{
	unsigned int num_threads = arch_num_cpus();

	spawn_threads(K_PRIO_COOP(12), num_threads, !EQUAL_PRIORITY,
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
	k_busy_wait(300000);

	for (i = 0; i < tnum; i++) {
		if (tinfo[i].executed == 1 && threads_woke_up <= tnum) {
			threads_woke_up++;
		}
	}
	zassert_equal(threads_woke_up, tnum, "Threads did not wakeup");
}

/**
 * @brief Verify that k_wakeup() resumes threads sleeping on other CPUs.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * k_wakeup() must cancel the timeout of a sleeping thread regardless of which
 * CPU that thread was last scheduled on, and the woken threads must then be
 * dispatched. The woken threads set a flag after their sleep returns, so the
 * test can confirm they resumed before their (much longer) sleep would have
 * expired on its own.
 *
 * Test steps:
 * - Spawn one cooperative thread per remaining CPU; each flags that it started
 *   and then sleeps.
 * - Spin until every thread has flagged its start and is blocked.
 * - Call k_wakeup() on each of them from the test thread.
 * - Busy-wait to let the other CPUs schedule them, then count the threads that
 *   ran past their sleep.
 * - Abort the spawned threads and clean up.
 *
 * Expected result:
 * - Every thread starts, and every thread resumes after k_wakeup().
 *
 * @see k_wakeup()
 * @see k_msleep()
 */
ZTEST(smp, test_smp_wakeup_threads)
{
	unsigned int num_threads = arch_num_cpus();

	/* Spawn threads to run on all remaining cores */
	spawn_threads(K_PRIO_COOP(12), num_threads - 1, !EQUAL_PRIORITY,
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

static int _cpu_id;

/**
 * @brief Verify that a thread can query the CPU record it is executing on.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * The architecture layer must provide a pointer to the kernel CPU record of
 * the CPU the caller is running on. The test thread records its own CPU id and
 * hands it to a spawned thread, which queries arch_curr_cpu() itself and
 * asserts that the record is valid and reports a different CPU — which is only
 * true if the spawned thread really was dispatched to another CPU.
 *
 * Test steps:
 * - Record the test thread's CPU id via arch_curr_cpu().
 * - Spawn a cooperative thread and pass it the recorded id.
 * - In the spawned thread, call arch_curr_cpu(), check the returned pointer is
 *   not NULL and that its id differs from the recorded one, then spin.
 * - Busy-wait, then abort and join the spawned thread.
 *
 * Expected result:
 * - arch_curr_cpu() returns a valid CPU record in the spawned thread, whose id
 *   differs from the test thread's CPU.
 *
 * @see arch_curr_cpu()
 */
ZTEST(smp, test_smp_get_cpu)
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

/**
 * @brief Verify the number of active CPUs honors the configured maximum
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * The maximum number of CPUs is configurable via CONFIG_MP_MAX_NUM_CPUS.
 * Verify that the number of CPUs the kernel brought up and reports through
 * arch_num_cpus() is at least one and never exceeds the configured maximum.
 *
 * Test steps:
 * - Query the number of active CPUs with arch_num_cpus().
 * - Compare it against the range [1, CONFIG_MP_MAX_NUM_CPUS].
 *
 * Expected result:
 * - arch_num_cpus() reports a count within the configured range.
 *
 * @see arch_num_cpus()
 */
ZTEST(smp, test_smp_num_cpus)
{
	unsigned int num_cpus = arch_num_cpus();

	zassert_between_inclusive(num_cpus, 1, CONFIG_MP_MAX_NUM_CPUS,
				  "active CPUs (%u) outside the configured range [1, %d]",
				  num_cpus, CONFIG_MP_MAX_NUM_CPUS);
}

#ifdef CONFIG_TRACE_SCHED_IPI
/* global variable for testing send IPI */
static volatile int sched_ipi_has_called;

void z_trace_sched_ipi(void)
{
	sched_ipi_has_called++;
}
#endif

#if defined(CONFIG_SCHED_IPI_SUPPORTED) || defined(__DOXYGEN__)
/**
 * @brief Verify that a broadcast scheduler IPI reaches the other CPUs.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * The architecture layer must be able to issue an interprocessor interrupt to
 * every other CPU in the system, which those CPUs then handle as a scheduler
 * IPI. The test hooks z_trace_sched_ipi(), which the scheduler IPI handler
 * calls on the receiving CPU, so an incremented counter proves the IPI was
 * both delivered and processed elsewhere. Skipped when the trace hook is not
 * built in, as there is then no way to observe delivery.
 *
 * Test steps:
 * - Clear the IPI counter.
 * - Call arch_sched_broadcast_ipi().
 * - Sleep in a retry loop until the counter becomes non-zero or the retries
 *   are exhausted.
 * - Repeat CONFIG_SMP_IPI_NUM_ITERS times.
 *
 * Expected result:
 * - The scheduler IPI handler runs at least once on another CPU in every
 *   iteration.
 *
 * @see arch_sched_broadcast_ipi()
 * @see z_trace_sched_ipi()
 */
ZTEST(smp, test_smp_ipi)
{
#ifndef CONFIG_TRACE_SCHED_IPI
	ztest_test_skip();
#else
	TC_PRINT("There are %u CPUs.\n", arch_num_cpus());

	for (int i = 0; i < CONFIG_SMP_IPI_NUM_ITERS; i++) {
		int retries = CONFIG_SMP_IPI_WAIT_RETRIES;
		/* issue a sched ipi to tell other CPU to run thread */
		sched_ipi_has_called = 0;
		arch_sched_broadcast_ipi();

		/* Need to wait longer than we think, loaded CI
		 * systems need to wait for host scheduling to run the
		 * other CPU's thread.
		 */
		while (retries > 0) {
			k_msleep(CONFIG_SMP_IPI_WAIT_MS);

			/* Bail out early if test is deemed a success. */
			if (sched_ipi_has_called > 0) {
				break;
			}

			retries--;
		}

		/**TESTPOINT: check if enter our IPI interrupt handler */
		zassert_true(sched_ipi_has_called != 0,
				"did not receive IPI.(%d,%d)", i,
				sched_ipi_has_called);
	}
#endif
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
 * @brief Verify that fatal errors are handled per CPU.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * On an SMP system a fatal error may be raised concurrently on more than one
 * CPU, and each CPU must run the fatal error handler for the thread it was
 * executing. A child thread and the test thread each trigger a kernel oops;
 * the fatal error handler records the CPU it was entered on and checks that
 * the two crashes were handled on different CPUs.
 *
 * Test steps:
 * - Create a preemptible child thread whose entry point calls k_oops().
 * - Busy-wait without rescheduling and confirm the child thread is dead.
 * - Call k_oops() from the test thread itself.
 * - In the fatal error handler, record the handling CPU for both crashes and
 *   compare them.
 *
 * Expected result:
 * - Both oopses are reported as K_ERR_KERNEL_OOPS, the child thread is
 *   terminated, and the two fatal errors are handled on different CPUs.
 *
 * @see k_oops()
 * @see k_sys_fatal_error_handler()
 */
ZTEST(smp, test_smp_fatal_error)
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
 * @brief Verify that the system workqueue runs on a different CPU.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * The system workqueue thread is an ordinary kernel thread and is therefore
 * eligible to be scheduled on any CPU. With the test thread occupying one CPU
 * and never blocking, a submitted work item must still be processed, which is
 * only possible on another CPU. The handler records the CPU it ran on so the
 * placement can be checked.
 *
 * Test steps:
 * - Initialize a work item whose handler records its CPU id.
 * - Submit it to the system workqueue with k_work_submit().
 * - Busy-wait without giving up the current CPU.
 * - Check that the work item is no longer busy and compare the recorded CPU id
 *   against the test thread's.
 *
 * Expected result:
 * - The work item completes, and its handler ran on a CPU other than the one
 *   running the test thread.
 *
 * @see k_work_submit()
 * @see k_work_busy_get()
 */
ZTEST(smp, test_smp_workq)
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
 * @brief Verify that a thread pending on a mutex releases the global lock.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * When a thread is switched out, the SMP layer calls
 * z_smp_release_global_lock() to drop any global lock the outgoing thread
 * still holds. A thread that did not take an irq_lock() must leave its
 * per-thread global lock count at zero across that switch. Two threads
 * contend for the same mutex on another CPU so the release path is taken, and
 * the second thread asserts its lock count before, between and after its
 * lock/unlock pair.
 *
 * Test steps:
 * - Create a lower-priority thread that takes the mutex, sleeps and unlocks it.
 * - Create a higher-priority thread that pends on the same mutex.
 * - Busy-wait on the current CPU so the contention resolves on another CPU.
 * - Join both threads and clean up.
 *
 * Expected result:
 * - The contending thread's global lock count stays zero throughout, and both
 *   threads terminate without deadlocking.
 *
 * @see k_mutex_lock()
 * @see k_mutex_unlock()
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
 * @brief Verify that the locking primitives serialize updates across CPUs.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * Three threads increment a shared counter concurrently on different CPUs,
 * each guarding the update with the primitive under test. If the primitive
 * provides mutual exclusion, no update is lost and the final count is exactly
 * three times the loop count; a lost update or a deadlock fails the test. The
 * scenario is repeated for the global IRQ lock, a semaphore and a mutex.
 *
 * Test steps:
 * - Reset the shared counter and select the locking primitive.
 * - Spawn three preemptible threads that each take the lock, update the shared
 *   counter and release the lock, LOOP_COUNT times.
 * - Join all three threads and compare the counter against 3 * LOOP_COUNT.
 * - Repeat for irq_lock(), k_sem_take()/k_sem_give() and
 *   k_mutex_lock()/k_mutex_unlock().
 *
 * Expected result:
 * - For every primitive the final count equals 3 * LOOP_COUNT and no deadlock
 *   occurs.
 *
 * @see irq_lock()
 * @see k_sem_take()
 * @see k_mutex_lock()
 */
ZTEST(smp, test_smp_inc_concurrency)
{
	if (LOOP_COUNT == 0) {
		/* If LOOP_COUNT is zero, the spawned threads are not looping
		 * at all. So skip this test.
		 */
		ztest_test_skip();
	}

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

/* Keep track of how many signals raised. */
static unsigned int t_signal_raised;

/* Keep track of how many signals received per thread. */
static unsigned int t_signals_rcvd[MAX_NUM_THREADS];

/* Worker body: waits on its own poll event, validates the raised signal and
 * resets both event and signal so the raiser can signal it again.
 */
static void process_events(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	uintptr_t id = (uintptr_t) arg0;
	unsigned int signaled;
	int result;

	while (1) {
		/* Retry if no event(s) are ready.
		 * For example, -EINTR where polling is interrupted.
		 */
		if (k_poll(&tevent[id], 1, K_FOREVER) != 0) {
			continue;
		}

		/* Grab the raised signal. */
		k_poll_signal_check(tevent[id].signal, &signaled, &result);

		/* Check correct result. */
		if (result != 0x55) {
			ztest_test_fail();
		}

		t_signals_rcvd[id]++;

		/* Reset both event and signal. */
		tevent[id].state = K_POLL_STATE_NOT_READY;
		tevent[id].signal->result = 0;
		k_poll_signal_reset(tevent[id].signal);
	}
}

static void signal_raise(void *arg0, void *arg1, void *arg2)
{
	unsigned int num_threads = arch_num_cpus();
	unsigned int signaled;
	int result;

	t_signal_raised = 0U;

	while (1) {
		for (uintptr_t i = 0; i < num_threads; i++) {
			/* Only raise signal when it is okay to do so.
			 * We don't want to raise a signal while the signal
			 * and the associated event are still in the process
			 * of being reset (see above).
			 */
			k_poll_signal_check(tevent[i].signal, &signaled, &result);

			if (signaled != 0U) {
				continue;
			}

			t_signal_raised++;
			k_poll_signal_raise(&tsignal[i], 0x55);
		}
	}
}

/**
 * @brief Stress the context switching code across all CPUs via k_poll signals.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * The polling API is used to hammer every CPU with thread swapping requests:
 * one worker thread per CPU blocks in k_poll(), while a cooperative thread
 * raises their signals in a tight loop. Sustaining this for several seconds
 * exercises the SMP context switch paths under contention; a lost wakeup, a
 * corrupted signal or a hang shows up as a thread that received no signals at
 * all. Skipped when CONFIG_SMP_TEST_RUN_FACTOR is zero, which reduces the run
 * time to nothing.
 *
 * Test steps:
 * - Initialize one poll signal and event per CPU and spawn a worker thread for
 *   each, at distinct preemptible priorities.
 * - Spawn a cooperative thread that continuously raises every signal.
 * - Sleep for the configured run time, then abort and join all threads.
 * - Check the per-thread count of received signals.
 *
 * Expected result:
 * - Every worker thread received at least one signal, and all threads abort
 *   and join cleanly.
 *
 * @see k_poll()
 * @see k_poll_signal_raise()
 * @see k_poll_signal_reset()
 */
ZTEST(smp_stress, test_smp_switch_stress)
{
	unsigned int num_threads = arch_num_cpus();

	if (CONFIG_SMP_TEST_RUN_FACTOR == 0) {
		/* If CONFIG_SMP_TEST_RUN_FACTOR is zero,
		 * the switch stress test is effectively
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
		t_signals_rcvd[i] = 0;

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

	TC_PRINT("Total signals raised %u\n", t_signal_raised);

	for (unsigned int i = 0; i < num_threads; i++) {
		TC_PRINT("Thread #%d received %u signals\n", i, t_signals_rcvd[i]);
	}

	/* Check if we at least have done some switching. */
	for (unsigned int i = 0; i < num_threads; i++) {
		zassert_not_equal(0, t_signals_rcvd[i],
				  "Thread #%d has not received any signals", i);
	}
}

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
ZTEST_SUITE(smp_stress, NULL, smp_tests_setup, NULL, NULL, NULL);

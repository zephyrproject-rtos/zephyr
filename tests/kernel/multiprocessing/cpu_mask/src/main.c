/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <ksched.h>

#if CONFIG_MP_MAX_NUM_CPUS < 2
#error "CPU mask test requires at least two CPUs"
#endif

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define NUM_THREADS CONFIG_MP_MAX_NUM_CPUS
#define DELAY_US    50000U

/**
 * @brief Tests for the per-thread CPU mask / CPU affinity scheduler APIs
 *
 * @defgroup kernel_cpu_mask_tests CPU Mask
 *
 * @ingroup all_tests
 *
 * This module tests the per-thread CPU affinity APIs:
 * k_thread_cpu_mask_clear(), k_thread_cpu_mask_enable_all(),
 * k_thread_cpu_mask_enable(), k_thread_cpu_mask_disable() and
 * k_thread_cpu_pin().
 * @{
 * @}
 */

/* Stacks and thread objects for worker threads */
static struct k_thread worker_threads[NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, NUM_THREADS, STACK_SIZE);

/* Per-thread execution tracking */
static volatile int worker_ran[NUM_THREADS];
static volatile int worker_cpu[NUM_THREADS];

K_SEM_DEFINE(worker_sem, 0, NUM_THREADS);

static int curr_cpu(void)
{
	unsigned int k = arch_irq_lock();
	int id = arch_curr_cpu()->id;

	arch_irq_unlock(k);
	return id;
}

static void worker_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int idx = POINTER_TO_INT(p1);

	worker_cpu[idx] = curr_cpu();
	worker_ran[idx] = 1;
	k_sem_give(&worker_sem);
}

static void reset_state(void)
{
	unsigned int ncpus = arch_num_cpus();

	k_sem_reset(&worker_sem);
	for (unsigned int i = 0U; i < ncpus; i++) {
		worker_ran[i] = 0;
		worker_cpu[i] = -1;
	}
}

/**
 * @brief Verify that the cpu_mask APIs reject a running thread with -EINVAL.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * The CPU affinity APIs may only modify a thread that is prevented from
 * running. Operating on a thread that is already running (here, the calling
 * thread itself) must be rejected. In PIN_ONLY mode cpu_mask_mod() uses
 * __ASSERT rather than the soft -EINVAL return, so the test is skipped in
 * that configuration.
 *
 * Test steps:
 * - Obtain the currently running thread via k_current_get().
 * - Invoke each cpu_mask API (clear, enable_all, enable, disable, pin) on it.
 * - Check the return value of every call.
 *
 * Expected result:
 * - Every cpu_mask API returns -EINVAL.
 *
 * @see k_thread_cpu_mask_clear()
 * @see k_thread_cpu_mask_enable_all()
 * @see k_thread_cpu_mask_enable()
 * @see k_thread_cpu_mask_disable()
 * @see k_thread_cpu_pin()
 * @verifies ZEP-SRS-34-13
 */
ZTEST(cpu_mask, test_api_rejects_running_thread)
{
	/* PIN_ONLY places __ASSERT before the soft -EINVAL path in
	 * cpu_mask_mod(), so calling any mask API on a running thread
	 * would panic rather than return -EINVAL.
	 */
	if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		ztest_test_skip();
	}

	k_tid_t self = k_current_get();
	int ret;

	ret = k_thread_cpu_mask_clear(self);
	zassert_equal(ret, -EINVAL, "expected -EINVAL, got %d", ret);

	ret = k_thread_cpu_mask_enable_all(self);
	zassert_equal(ret, -EINVAL, "expected -EINVAL, got %d", ret);

	ret = k_thread_cpu_mask_enable(self, 0);
	zassert_equal(ret, -EINVAL, "expected -EINVAL, got %d", ret);

	ret = k_thread_cpu_mask_disable(self, 0);
	zassert_equal(ret, -EINVAL, "expected -EINVAL, got %d", ret);

	ret = k_thread_cpu_pin(self, 0);
	zassert_equal(ret, -EINVAL, "expected -EINVAL, got %d", ret);
}

/**
 * @brief Verify that a thread with a cleared CPU mask never executes.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * Clearing a thread's CPU mask leaves it ineligible to run on any CPU, so the
 * scheduler must never dispatch it. Skipped in PIN_ONLY mode, where
 * cpu_mask_mod() asserts that exactly one mask bit is set and a clear would
 * panic.
 *
 * Test steps:
 * - Create a high-priority thread in the K_FOREVER (not-started) state.
 * - Clear its CPU mask with k_thread_cpu_mask_clear().
 * - Start the thread and sleep to give the scheduler a chance to run it.
 * - Inspect the per-thread "ran" flag.
 *
 * Expected result:
 * - The thread never runs (its ran flag stays 0).
 *
 * @see k_thread_cpu_mask_clear()
 * @verifies ZEP-SRS-34-2
 */
ZTEST(cpu_mask, test_mask_clear_prevents_execution)
{
	/* k_thread_cpu_mask_clear() sets the mask to zero.  In PIN_ONLY mode
	 * cpu_mask_mod() asserts that the mask is a single set bit after every
	 * modification, so a clear would panic rather than succeed.
	 */
	if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		ztest_test_skip();
	}

	k_tid_t tid;
	int prio = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	reset_state();

	tid = k_thread_create(&worker_threads[0], worker_stacks[0],
			      STACK_SIZE, worker_fn,
			      INT_TO_POINTER(0), NULL, NULL,
			      prio, 0, K_FOREVER);

	ret = k_thread_cpu_mask_clear(tid);
	zassert_equal(ret, 0, "k_thread_cpu_mask_clear failed: %d", ret);

	k_thread_start(tid);
	k_msleep(100);

	zassert_equal(worker_ran[0], 0,
		      "thread ran despite cleared CPU mask");

	k_thread_abort(tid);
	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify that k_thread_cpu_mask_enable_all() restores eligibility.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * After a thread's mask has been cleared, enable_all() must re-enable every
 * CPU bit so the thread becomes runnable again. Skipped in PIN_ONLY mode,
 * which permits only a single CPU bit.
 *
 * Test steps:
 * - Create a high-priority thread in the K_FOREVER state.
 * - Clear its CPU mask, then call k_thread_cpu_mask_enable_all().
 * - Start the thread and wait on the worker semaphore.
 *
 * Expected result:
 * - The thread runs: the semaphore take succeeds and the ran flag is set.
 *
 * @see k_thread_cpu_mask_enable_all()
 * @verifies ZEP-SRS-34-2
 */
ZTEST(cpu_mask, test_mask_enable_all_allows_execution)
{
	/* enable_all sets all CPU bits; PIN_ONLY allows only one bit. */
	if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		ztest_test_skip();
	}

	k_tid_t tid;
	int prio = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	reset_state();

	tid = k_thread_create(&worker_threads[0], worker_stacks[0],
			      STACK_SIZE, worker_fn,
			      INT_TO_POINTER(0), NULL, NULL,
			      prio, 0, K_FOREVER);

	ret = k_thread_cpu_mask_clear(tid);
	zassert_equal(ret, 0, "k_thread_cpu_mask_clear failed: %d", ret);

	ret = k_thread_cpu_mask_enable_all(tid);
	zassert_equal(ret, 0, "k_thread_cpu_mask_enable_all failed: %d", ret);

	k_thread_start(tid);

	ret = k_sem_take(&worker_sem, K_MSEC(2000));
	zassert_equal(ret, 0, "thread did not run after enable_all");

	zassert_equal(worker_ran[0], 1, "thread flag not set");

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify that disabling a CPU in the mask excludes that CPU.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * Disabling the current CPU in a thread's mask must prevent the scheduler
 * from dispatching it on that CPU; with at least one other CPU available the
 * thread must instead run elsewhere. Skipped in PIN_ONLY mode, where leaving
 * more than one bit set is not permitted.
 *
 * Test steps:
 * - Record the current CPU id.
 * - Create a high-priority thread in the K_FOREVER state.
 * - Disable the current CPU in its mask with k_thread_cpu_mask_disable().
 * - Start the thread and wait for it to run.
 * - Read back the CPU id the thread recorded.
 *
 * Expected result:
 * - The thread runs, and it runs on a CPU other than the excluded one.
 *
 * @see k_thread_cpu_mask_disable()
 * @verifies ZEP-SRS-34-2
 */
ZTEST(cpu_mask, test_mask_disable_local_cpu)
{
	/* disable() on a thread with all CPUs enabled leaves more than one bit
	 * set in the mask, which PIN_ONLY does not permit.
	 */
	if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		ztest_test_skip();
	}

	k_tid_t tid;
	int local = curr_cpu();
	int prio = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	reset_state();

	tid = k_thread_create(&worker_threads[0], worker_stacks[0],
			      STACK_SIZE, worker_fn,
			      INT_TO_POINTER(0), NULL, NULL,
			      prio, 0, K_FOREVER);

	ret = k_thread_cpu_mask_disable(tid, local);
	zassert_equal(ret, 0, "k_thread_cpu_mask_disable failed: %d", ret);

	k_thread_start(tid);

	ret = k_sem_take(&worker_sem, K_MSEC(2000));
	zassert_equal(ret, 0, "thread did not run after disabling local CPU");

	zassert_not_equal(worker_cpu[0], local,
			  "thread ran on excluded CPU %d", local);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify that k_thread_cpu_pin() constrains a thread to one CPU.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * Pinning a thread to a specific CPU must force the scheduler to dispatch it
 * only on that CPU. Pinning to a CPU other than the current one makes the
 * constraint observable.
 *
 * Test steps:
 * - Compute a target CPU id that differs from the current CPU.
 * - Create a high-priority thread in the K_FOREVER state.
 * - Pin it to the target CPU with k_thread_cpu_pin().
 * - Start the thread, wait for it to run, and read back its recorded CPU id.
 *
 * Expected result:
 * - The thread runs on the target CPU.
 *
 * @see k_thread_cpu_pin()
 * @verifies ZEP-SRS-34-12
 */
ZTEST(cpu_mask, test_cpu_pin_runs_on_target)
{
	unsigned int ncpus = arch_num_cpus();
	k_tid_t tid;
	int local = curr_cpu();
	int target = (local + 1) % (int)ncpus;
	int prio = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	reset_state();

	tid = k_thread_create(&worker_threads[0], worker_stacks[0],
			      STACK_SIZE, worker_fn,
			      INT_TO_POINTER(0), NULL, NULL,
			      prio, 0, K_FOREVER);

	ret = k_thread_cpu_pin(tid, target);
	zassert_equal(ret, 0, "k_thread_cpu_pin failed: %d", ret);

	k_thread_start(tid);

	ret = k_sem_take(&worker_sem, K_MSEC(2000));
	zassert_equal(ret, 0, "pinned thread did not run");

	zassert_equal(worker_cpu[0], target,
		      "thread ran on CPU %d instead of pinned CPU %d",
		      worker_cpu[0], target);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify that threads pinned to distinct CPUs each run on their CPU.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * When one thread is pinned to each CPU, the scheduler must honour every
 * pin simultaneously and place each thread on its own CPU.
 *
 * Test steps:
 * - For each available CPU i, create a thread in the K_FOREVER state and pin
 *   it to CPU i with k_thread_cpu_pin().
 * - Start every thread.
 * - Wait on the worker semaphore once per thread.
 * - Read back the CPU id each thread recorded.
 *
 * Expected result:
 * - Every thread runs, and thread i runs on CPU i.
 *
 * @see k_thread_cpu_pin()
 * @verifies ZEP-SRS-34-12
 */
ZTEST(cpu_mask, test_pin_each_thread_to_distinct_cpu)
{
	unsigned int ncpus = arch_num_cpus();
	int prio = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	reset_state();

	for (unsigned int i = 0U; i < ncpus; i++) {
		k_thread_create(&worker_threads[i], worker_stacks[i],
				STACK_SIZE, worker_fn,
				INT_TO_POINTER(i), NULL, NULL,
				prio, 0, K_FOREVER);

		ret = k_thread_cpu_pin(&worker_threads[i], (int)i);
		zassert_equal(ret, 0,
			      "k_thread_cpu_pin(%u) failed: %d", i, ret);
	}

	for (unsigned int i = 0U; i < ncpus; i++) {
		k_thread_start(&worker_threads[i]);
	}

	for (unsigned int i = 0U; i < ncpus; i++) {
		ret = k_sem_take(&worker_sem, K_MSEC(5000));
		zassert_equal(ret, 0,
			      "timed out waiting for thread %u", i);
	}

	for (unsigned int i = 0U; i < ncpus; i++) {
		zassert_equal(worker_cpu[i], (int)i,
			      "thread %u ran on CPU %d, expected %u",
			      i, worker_cpu[i], i);
	}

	for (unsigned int i = 0U; i < ncpus; i++) {
		k_thread_join(&worker_threads[i], K_FOREVER);
	}
}

/**
 * @brief Verify per-CPU enable/disable toggles a thread's eligibility.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * Enabling then disabling the only set CPU bit must leave the thread
 * ineligible to run; a later enable_all() must make it runnable again.
 * Mask modifications require the thread to be prevented from running, so the
 * thread is suspended before re-enabling. Skipped in PIN_ONLY mode, which
 * permits only a single CPU bit.
 *
 * Test steps:
 * - Create a high-priority thread in the K_FOREVER state.
 * - Clear its mask, enable CPU 0, then disable CPU 0.
 * - Start the thread and sleep; confirm it does not run.
 * - Suspend the thread, call k_thread_cpu_mask_enable_all(), resume it.
 * - Wait on the worker semaphore.
 *
 * Expected result:
 * - The thread does not run while all CPUs are disabled.
 * - The thread runs after the mask is re-enabled.
 *
 * @see k_thread_cpu_mask_enable()
 * @see k_thread_cpu_mask_disable()
 * @verifies ZEP-SRS-34-2
 */
ZTEST(cpu_mask, test_individual_cpu_enable_disable)
{
	/* enable_all sets all CPU bits; PIN_ONLY allows only one bit. */
	if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		ztest_test_skip();
	}

	k_tid_t tid;
	int prio = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	reset_state();

	tid = k_thread_create(&worker_threads[0], worker_stacks[0],
			      STACK_SIZE, worker_fn,
			      INT_TO_POINTER(0), NULL, NULL,
			      prio, 0, K_FOREVER);

	/* Clear mask, then enable and immediately disable CPU 0 */
	ret = k_thread_cpu_mask_clear(tid);
	zassert_equal(ret, 0, "clear failed: %d", ret);

	ret = k_thread_cpu_mask_enable(tid, 0);
	zassert_equal(ret, 0, "enable failed: %d", ret);

	ret = k_thread_cpu_mask_disable(tid, 0);
	zassert_equal(ret, 0, "disable failed: %d", ret);

	k_thread_start(tid);
	k_msleep(100);

	zassert_equal(worker_ran[0], 0,
		      "thread ran with all CPUs disabled");

	/* Suspend the thread so it is prevented-from-running, which is
	 * required by the cpu_mask API before modifying the mask.
	 */
	k_thread_suspend(tid);

	ret = k_thread_cpu_mask_enable_all(tid);
	zassert_equal(ret, 0, "enable_all failed: %d", ret);

	k_thread_resume(tid);

	ret = k_sem_take(&worker_sem, K_MSEC(2000));
	zassert_equal(ret, 0, "thread never ran after re-enabling CPUs");

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify that a cooperative thread honours its CPU pin.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * CPU pinning must apply to cooperative-priority threads as well as
 * preemptible ones: a pinned cooperative thread must run only on its target
 * CPU.
 *
 * Test steps:
 * - Compute a target CPU id that differs from the current CPU.
 * - Create a cooperative-priority thread (K_PRIO_COOP) in the K_FOREVER state.
 * - Pin it to the target CPU with k_thread_cpu_pin().
 * - Start the thread, wait for it to run, and read back its recorded CPU id.
 *
 * Expected result:
 * - The thread runs on the target CPU.
 *
 * @see k_thread_cpu_pin()
 * @verifies ZEP-SRS-34-12
 */
ZTEST(cpu_mask, test_coop_thread_pinned_cpu)
{
	unsigned int ncpus = arch_num_cpus();
	int local = curr_cpu();
	int target = (local + 1) % (int)ncpus;
	k_tid_t tid;
	int ret;

	reset_state();

	tid = k_thread_create(&worker_threads[0], worker_stacks[0],
			      STACK_SIZE, worker_fn,
			      INT_TO_POINTER(0), NULL, NULL,
			      K_PRIO_COOP(2), 0, K_FOREVER);

	ret = k_thread_cpu_pin(tid, target);
	zassert_equal(ret, 0, "k_thread_cpu_pin failed: %d", ret);

	k_thread_start(tid);

	ret = k_sem_take(&worker_sem, K_MSEC(2000));
	zassert_equal(ret, 0, "cooperative pinned thread did not run");

	zassert_equal(worker_cpu[0], target,
		      "thread ran on CPU %d, expected %d",
		      worker_cpu[0], target);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify that PIN_ONLY mode supports single-CPU pinning.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * When CONFIG_SCHED_CPU_MASK_PIN_ONLY is selected, each thread must be bound
 * to exactly one CPU. This test exercises the supported pin path in that
 * configuration and is skipped when PIN_ONLY is not enabled.
 *
 * Test steps:
 * - Create a high-priority thread in the K_FOREVER state.
 * - Pin it to CPU 0 with k_thread_cpu_pin().
 * - Start the thread, wait for it to run, and read back its recorded CPU id.
 *
 * Expected result:
 * - The thread runs only on CPU 0.
 *
 * @see k_thread_cpu_pin()
 * @verifies ZEP-SRS-34-15
 */
ZTEST(cpu_mask, test_pin_only_single_cpu)
{
	if (!IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		ztest_test_skip();
	}

	k_tid_t tid;
	int prio = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	reset_state();

	tid = k_thread_create(&worker_threads[0], worker_stacks[0],
			      STACK_SIZE, worker_fn,
			      INT_TO_POINTER(0), NULL, NULL,
			      prio, 0, K_FOREVER);

	ret = k_thread_cpu_pin(tid, 0);
	zassert_equal(ret, 0, "k_thread_cpu_pin failed: %d", ret);

	k_thread_start(tid);

	ret = k_sem_take(&worker_sem, K_MSEC(2000));
	zassert_equal(ret, 0, "pinned thread did not run");

	zassert_equal(worker_cpu[0], 0,
		      "thread ran on CPU %d, expected 0", worker_cpu[0]);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify that a pinned thread does not starve an unpinned thread.
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * An affinity constraint on one thread must not prevent other threads from
 * being scheduled on the remaining CPUs. A thread pinned to CPU 0 and a
 * second thread eligible for all CPUs must both complete. Skipped in
 * PIN_ONLY mode, where the "free" thread's enable_all() is not permitted.
 *
 * Test steps:
 * - Create a thread pinned to CPU 0 with k_thread_cpu_pin().
 * - Create a second thread made eligible for all CPUs via enable_all().
 * - Start both threads and wait on the worker semaphore once per thread.
 * - Inspect both ran flags and the pinned thread's recorded CPU id.
 *
 * Expected result:
 * - Both threads run; the pinned thread runs on CPU 0.
 *
 * @see k_thread_cpu_pin()
 * @see k_thread_cpu_mask_enable_all()
 * @verifies ZEP-SRS-34-12
 */
ZTEST(cpu_mask, test_pinned_and_free_thread_coexist)
{
	/* The "free" thread uses enable_all which sets all CPU bits;
	 * PIN_ONLY allows only one bit.
	 */
	if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		ztest_test_skip();
	}

	k_tid_t pinned, unmasked;
	int prio = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	reset_state();

	pinned = k_thread_create(&worker_threads[0], worker_stacks[0],
				 STACK_SIZE, worker_fn,
				 INT_TO_POINTER(0), NULL, NULL,
				 prio, 0, K_FOREVER);

	ret = k_thread_cpu_pin(pinned, 0);
	zassert_equal(ret, 0, "k_thread_cpu_pin failed: %d", ret);

	unmasked = k_thread_create(&worker_threads[1], worker_stacks[1],
				   STACK_SIZE, worker_fn,
				   INT_TO_POINTER(1), NULL, NULL,
				   prio, 0, K_FOREVER);

	ret = k_thread_cpu_mask_enable_all(unmasked);
	zassert_equal(ret, 0, "enable_all failed: %d", ret);

	k_thread_start(pinned);
	k_thread_start(unmasked);

	for (int i = 0; i < 2; i++) {
		ret = k_sem_take(&worker_sem, K_MSEC(5000));
		zassert_equal(ret, 0,
			      "timed out waiting for thread %d", i);
	}

	zassert_equal(worker_ran[0], 1, "pinned thread did not run");
	zassert_equal(worker_ran[1], 1, "unmasked thread did not run");
	zassert_equal(worker_cpu[0], 0,
		      "pinned thread ran on CPU %d, expected 0",
		      worker_cpu[0]);

	k_thread_join(pinned, K_FOREVER);
	k_thread_join(unmasked, K_FOREVER);
}

/* Worker body for test_pin_affinity_across_yield: re-checks that it is still
 * on its pinned CPU after each of 30 k_yield() calls.
 */
static void check_affinity(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	int affinity = POINTER_TO_INT(arg0);
	int counter = 30;

	while (counter != 0) {
		zassert_equal(affinity, curr_cpu(),
			      "affinity lost after yield: on CPU %d, expected %d",
			      curr_cpu(), affinity);
		counter--;
		k_yield();
	}
}

/**
 * @brief Verify that CPU pinning persists across k_yield().
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details
 * A pinned thread must remain on its target CPU even after voluntarily
 * yielding the processor; the scheduler must always return it to the same
 * CPU rather than migrating it.
 *
 * Test steps:
 * - For each available CPU i, create a thread pinned to CPU i.
 * - Each worker thread yields 30 times, asserting after every yield that it
 *   is still running on its pinned CPU.
 * - Join all worker threads.
 *
 * Expected result:
 * - Every thread observes its pinned CPU after each yield (no migration).
 *
 * @see k_thread_cpu_pin()
 * @see k_yield()
 * @verifies ZEP-SRS-34-12
 */
ZTEST(cpu_mask, test_pin_affinity_across_yield)
{
	unsigned int ncpus = arch_num_cpus();

	for (unsigned int i = 0U; i < ncpus; i++) {
		k_thread_create(&worker_threads[i], worker_stacks[i],
				STACK_SIZE, check_affinity,
				INT_TO_POINTER(i), NULL, NULL,
				0, 0U, K_FOREVER);

		(void)k_thread_cpu_pin(&worker_threads[i], (int)i);
		k_thread_start(&worker_threads[i]);
	}

	for (unsigned int i = 0U; i < ncpus; i++) {
		k_thread_join(&worker_threads[i], K_FOREVER);
	}
}

ZTEST_SUITE(cpu_mask, NULL, NULL, NULL, NULL, NULL);

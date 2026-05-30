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
 * @brief Verify that cpu_mask API returns -EINVAL on a running thread
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details The cpu_mask APIs must refuse operations on a thread that is
 * already running (i.e. the calling thread itself).  In PIN_ONLY mode
 * cpu_mask_mod() uses __ASSERT rather than a soft -EINVAL return, so
 * the test is skipped entirely in that configuration.
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
 * @brief Verify that a thread with a cleared CPU mask never executes
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Create a high-priority thread, clear its cpu_mask so it is
 * not eligible for any CPU, start it, yield, and confirm it never ran.
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
 * @brief Verify that k_thread_cpu_mask_enable_all allows execution
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Clear the mask then call enable_all.  The thread must run.
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
 * @brief Verify that disabling the local CPU prevents the thread from
 *        running on it and that it runs on another CPU
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Disable the current CPU in the new thread's mask.  Since
 * there is at least one other CPU, the thread must eventually be
 * scheduled there.  Confirm the CPU id recorded by the thread is not
 * the one we excluded.
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
 * @brief Verify that k_thread_cpu_pin() constrains execution to one CPU
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Pin a thread to a CPU that is not the current one.  The
 * thread must run exclusively on that CPU.
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
 * @brief Verify that each thread, when pinned to a distinct CPU, runs
 *        on exactly that CPU
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Create one thread per available CPU, pin thread i to CPU i,
 * start all threads, wait for completion and confirm that every thread
 * recorded the expected CPU id.
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
 * @brief Verify enable/disable of individual CPUs in the mask
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Clear the mask, then enable CPU 0, then disable CPU 0.
 * The thread must not run.  Re-enable CPU 0 and confirm it runs.
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
 * @brief Verify affinity under a cooperative priority — the thread
 *        must not be preempted away from its pinned CPU while running
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Pin a cooperative thread to a specific CPU and confirm it
 * runs and records the correct CPU.
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
 * @brief Verify that PIN_ONLY mode enforces single-CPU pinning
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details When CONFIG_SCHED_CPU_MASK_PIN_ONLY is selected, every
 * thread must be assigned to exactly one CPU.  Create a thread, pin
 * it to CPU 0, start it and confirm it runs only on CPU 0.
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
 * @brief Verify that pinning a thread does not block threads on
 *        other CPUs from running concurrently
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Pin one thread to CPU 0 and a second thread with
 * enable_all.  Both must complete, demonstrating that affinity
 * constraints on one thread do not starve others.
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

/**
 * @brief Verify that CPU pinning persists across k_yield()
 *
 * @ingroup kernel_cpu_mask_tests
 *
 * @details Pin one thread per available CPU to its corresponding CPU.
 * Each thread yields 30 times and re-checks its CPU after every yield
 * to confirm the scheduler always returns it to the pinned CPU.
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

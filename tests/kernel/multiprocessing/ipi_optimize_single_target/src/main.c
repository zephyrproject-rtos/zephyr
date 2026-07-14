/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests for CONFIG_IPI_OPTIMIZE_SINGLE_TARGET, which selects a single "best"
 * CPU to receive an IPI instead of broadcasting to every eligible CPU.
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <ipi.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define NUM_THREADS (CONFIG_MP_MAX_NUM_CPUS - 1)

#define DELAY_FOR_IPIS 200
#define WAIT_FOR_THREAD_PENDING_US 100000

/* Priorities used to reproduce the "current CPU treated as a candidate"
 * regression: HIGH is the priority of the thread being made ready, MID is
 * the priority of the busy threads on the other CPUs (genuinely eligible
 * targets), and WORST is the priority given to the calling/current thread
 * itself, which must be preemptible and have a worse (numerically larger)
 * priority than every other candidate.
 */
#define REGR_HIGH_PRIO  0
#define REGR_MID_PRIO   (CONFIG_NUM_PREEMPT_PRIORITIES / 2)
#define REGR_WORST_PRIO (CONFIG_NUM_PREEMPT_PRIORITIES - 1)

static struct k_thread thread[NUM_THREADS];
static struct k_thread alt_thread;

static bool alt_thread_created;

static K_THREAD_STACK_ARRAY_DEFINE(stack, NUM_THREADS, STACK_SIZE);
static K_THREAD_STACK_DEFINE(alt_stack, STACK_SIZE);

static uint32_t ipi_count[CONFIG_MP_MAX_NUM_CPUS];
static struct k_spinlock ipilock;
static atomic_t busy_started;

static K_SEM_DEFINE(sem, 0, 1);

void z_trace_sched_ipi(void)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&ipilock);
	ipi_count[_current_cpu->id]++;
	k_spin_unlock(&ipilock, key);
}

static void clear_ipi_counts(void)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&ipilock);
	memset(ipi_count, 0, sizeof(ipi_count));
	k_spin_unlock(&ipilock, key);
}

static uint32_t total_ipi_count(void)
{
	k_spinlock_key_t  key;
	uint32_t  total = 0;

	key = k_spin_lock(&ipilock);
	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		total += ipi_count[i];
	}
	k_spin_unlock(&ipilock, key);

	return total;
}

static uint32_t count_cpus_with_ipi(void)
{
	k_spinlock_key_t  key;
	uint32_t  cpus = 0;

	key = k_spin_lock(&ipilock);
	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (ipi_count[i] > 0) {
			cpus++;
		}
	}
	k_spin_unlock(&ipilock, key);

	return cpus;
}

static void busy_thread_entry(void *p1, void *p2, void *p3)
{
	int  key;
	uint32_t id;

	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);

	atomic_or(&busy_started, BIT(id));

	while (1) {
	}
}

static bool wait_until_busy_threads_ready(unsigned int expect_count)
{
	uint32_t  value;
	unsigned int i;

	for (i = 0; i < 100; i++) {
		k_busy_wait(1000);

		value = (uint32_t)atomic_get(&busy_started);
		if (POPCOUNT(value) >= expect_count) {
			break;
		}
	}

	return (i < 100);
}

static volatile bool alt_thread_ran;

static void pending_thread_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem, K_FOREVER);

	/*
	 * Just flag that this thread ran and exit. It must not spin waiting
	 * on a flag set by the thread that created it: once this thread is
	 * made ready, it may immediately and synchronously preempt that
	 * creating thread locally on the same CPU (still inside its
	 * k_sem_give() call), so spinning here would deadlock.
	 */

	alt_thread_ran = true;
}

static void alt_thread_create(int priority, const char *desc)
{
	k_thread_create(&alt_thread, alt_stack, STACK_SIZE,
			pending_thread_entry, NULL, NULL, NULL,
			priority, 0, K_NO_WAIT);
	alt_thread_created = true;

	/* Verify alt_thread is pending */

	zassert_true(WAIT_FOR(z_is_thread_pending(&alt_thread),
				WAIT_FOR_THREAD_PENDING_US, k_msleep(1)),
		     "%s priority thread has not pended.\n", desc);
}

static uint32_t busy_threads_create(int priority)
{
	unsigned int  i;
	uint32_t      id;
	int           key;

	atomic_clear(&busy_started);

	/*
	 * Under CONFIG_IPI_OPTIMIZE_SINGLE_TARGET, ipi_mask_create() picks
	 * only the single best idle-ish CPU based on the current snapshot of
	 * _kernel.cpus[i].current. Creating every busy thread back-to-back
	 * before any of them have actually started would have every one of
	 * these k_thread_create() calls target the same CPU (since none of
	 * the earlier IPIs have been processed and _kernel.cpus[].current
	 * still shows the previous/idle thread everywhere else). Create and
	 * confirm each busy thread individually so each subsequent IPI
	 * target selection is made against an up-to-date snapshot.
	 */

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&thread[i], stack[i], STACK_SIZE,
				busy_thread_entry, NULL, NULL, NULL,
				priority, 0, K_NO_WAIT);

		zassert_true(wait_until_busy_threads_ready(i + 1),
			     "Busy thread %u not ready.\n", i);
	}

	/* Align to tick boundary to minimize probability of timer ISRs */

	k_sleep(K_TICKS(1));
	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);

	return id;
}

/**
 * Regression test for a bug in the single-target CPU search where the
 * calling/current CPU was not excluded from the candidate search. If the
 * current CPU's own thread is preemptible and has a worse priority than
 * every other candidate, it would win the search and the function would
 * return "no IPI needed" even though other CPUs were legitimately eligible
 * and should have been sent an IPI.
 *
 * This is only observable when the calling thread itself is preemptible, so
 * the calling (ztest) thread's priority is temporarily changed to a
 * preemptible priority that is deliberately worse than the busy threads
 * created on the other CPUs.
 */
ZTEST(ipi_optimize_single_target, test_current_cpu_not_treated_as_candidate)
{
	int  orig_prio;
	uint32_t  count;
	uint32_t  cpus_signaled;

	orig_prio = k_thread_priority_get(k_current_get());

	/*
	 * Set up alt_thread and the busy threads while the calling (ztest)
	 * thread is still at its normal, cooperative priority so that thread
	 * placement during setup is deterministic (the calling thread will
	 * not be preempted mid-setup).
	 */
	alt_thread_create(REGR_HIGH_PRIO, "Regression");

	(void)busy_threads_create(REGR_MID_PRIO);

	/* Let any IPIs related to thread creation above settle. */
	k_busy_wait(DELAY_FOR_IPIS);

	/*
	 * Now that every other thread involved in the test is confirmed
	 * placed and running/pending, make the calling thread preemptible
	 * with the worst priority of all threads involved in this test, so
	 * that (absent the fix) it would incorrectly win the "best
	 * candidate" search when alt_thread is made ready below.
	 */
	k_thread_priority_set(k_current_get(), REGR_WORST_PRIO);

	clear_ipi_counts();
	k_sem_give(&sem);
	k_busy_wait(DELAY_FOR_IPIS);

	count = total_ipi_count();
	cpus_signaled = count_cpus_with_ipi();

	k_thread_priority_set(k_current_get(), orig_prio);

	/* Let alt_thread run to completion now that priorities are restored. */
	zassert_true(WAIT_FOR(alt_thread_ran, WAIT_FOR_THREAD_PENDING_US, k_msleep(1)),
		     "High priority thread did not run to completion.\n");

	zassert_true(count > 0,
		     "No IPI was sent even though other CPUs were eligible "
		     "targets (current CPU was incorrectly treated as a "
		     "candidate).\n");

	/*
	 * NUM_THREADS busy threads (>= 2 for CONFIG_MP_MAX_NUM_CPUS >= 3) are
	 * all simultaneously eligible remote candidates: same priority, all
	 * preemptible, all better priority than the calling thread. Under
	 * CONFIG_IPI_OPTIMIZE_SINGLE_TARGET, exactly one of them may be
	 * targeted. If single-target selection regressed back to
	 * broadcasting to every eligible CPU (e.g. CONFIG_IPI_OPTIMIZE_SINGLE_TARGET=n),
	 * more than one CPU would show a nonzero IPI count here.
	 */
	zassert_equal(cpus_signaled, 1,
		      "Expected exactly 1 CPU to receive an IPI under "
		      "CONFIG_IPI_OPTIMIZE_SINGLE_TARGET, but %u CPUs were "
		      "signaled.\n", cpus_signaled);
}

static void *ipi_tests_setup(void)
{
	/*
	 * Sleep a bit to guarantee that all CPUs enter an idle thread
	 * from which they can exit correctly to run the test.
	 */

	k_sleep(K_MSEC(20));

	return NULL;
}

static void cleanup_threads(void *fixture)
{
	unsigned int  i;

	ARG_UNUSED(fixture);

	/*
	 * Ensure that spawned busy threads are aborted before
	 * proceeding to the next test.
	 */

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(&thread[i]);
	}

	/* Ensure alt_thread ,if it was created, also gets aborted */

	if (alt_thread_created) {
		k_thread_abort(&alt_thread);
	}
	alt_thread_created = false;

	alt_thread_ran = false;
}

ZTEST_SUITE(ipi_optimize_single_target, NULL, ipi_tests_setup, NULL, cleanup_threads, NULL);

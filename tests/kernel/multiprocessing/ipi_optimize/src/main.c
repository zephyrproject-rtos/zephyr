/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <ipi.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define NUM_THREADS (CONFIG_MP_MAX_NUM_CPUS - 1)

#define DELAY_FOR_IPIS 200
#define WAIT_FOR_IPIS_US 100000
#define WAIT_FOR_THREAD_PENDING_US 100000

static struct k_thread thread[NUM_THREADS];
static struct k_thread alt_thread;

static bool alt_thread_created;

static K_THREAD_STACK_ARRAY_DEFINE(stack, NUM_THREADS, STACK_SIZE);
static K_THREAD_STACK_DEFINE(alt_stack, STACK_SIZE);

static uint32_t ipi_count[CONFIG_MP_MAX_NUM_CPUS];
static struct k_spinlock ipilock;
static atomic_t busy_started;
static volatile bool alt_thread_done;

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

static void get_ipi_counts(uint32_t *set, size_t n_elem)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&ipilock);
	memcpy(set, ipi_count, n_elem * sizeof(*set));
	k_spin_unlock(&ipilock, key);
}

static bool ipi_counts_seen(uint32_t *set, size_t n_elem, uint32_t expected_mask)
{
	unsigned int i;

	get_ipi_counts(set, n_elem);

	for (i = 0; i < n_elem; i++) {
		if (((expected_mask & BIT(i)) != 0U) && (set[i] == 0U)) {
			return false;
		}
	}

	return true;
}

static bool wait_for_ipis(uint32_t *set, size_t n_elem, uint32_t expected_mask)
{
	return WAIT_FOR(ipi_counts_seen(set, n_elem, expected_mask),
			WAIT_FOR_IPIS_US, k_busy_wait(10));
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

static bool wait_until_busy_threads_ready(uint32_t id)
{
	uint32_t  all;
	uint32_t  value;
	unsigned int i;

	all = IPI_ALL_CPUS_MASK ^ BIT(id);
	for (i = 0; i < 10; i++) {
		k_busy_wait(1000);

		value = (uint32_t)atomic_get(&busy_started);
		if (value == all) {
			break;
		}
	}

	return (i < 10);
}

static void pending_thread_entry(void *p1, void *p2, void *p3)
{
	int  key;

	k_sem_take(&sem, K_FOREVER);

	while (!alt_thread_done) {
		key = arch_irq_lock();
		arch_spin_relax();
		arch_irq_unlock(key);
	}
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

uint32_t busy_threads_create(int priority)
{
	unsigned int  i;
	uint32_t      id;
	int           key;

	atomic_clear(&busy_started);

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&thread[i], stack[i], STACK_SIZE,
				busy_thread_entry, NULL, NULL, NULL,
				priority, 0, K_NO_WAIT);
	}

	/* Align to tick boundary to minimize probability of timer ISRs */

	k_sleep(K_TICKS(1));
	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);

	/*
	 * Spin until all busy threads are ready. It is assumed that as this
	 * thread and the busy threads are cooperative that they will not be
	 * rescheduled to execute on a different CPU.
	 */

	zassert_true(wait_until_busy_threads_ready(id),
		     "1 or more 'busy threads' not ready.\n");

	return id;
}

void busy_threads_priority_set(int priority, int delta)
{
	unsigned int  i;

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_priority_set(&thread[i], priority);
		priority += delta;
	}
}

/**
 * @brief Verify that arch_sched_broadcast_ipi() reaches every other CPU.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * A broadcast scheduler IPI must be delivered to all CPUs except the one that
 * issued it. Every CPU is kept busy by a cooperative thread so none of them is
 * idle, and the z_trace_sched_ipi() hook counts the IPIs each CPU receives.
 *
 * Test steps:
 * - Create one busy cooperative thread per other CPU and wait until all of
 *   them are running.
 * - Clear the per-CPU IPI counts.
 * - Call arch_sched_broadcast_ipi().
 * - Wait until the expected CPUs report an IPI, then read all counts.
 *
 * Expected result:
 * - Every CPU other than the issuing one received exactly one IPI, and the
 *   issuing CPU received none.
 *
 * @see arch_sched_broadcast_ipi()
 */
ZTEST(ipi, test_ipi_broadcast_reaches_all_cpus)
{
	uint32_t  set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t  id;
	int priority;
	unsigned int j;

	priority = k_thread_priority_get(k_current_get());

	id = busy_threads_create(priority - 1);

	/* Broadcast the IPI. All other CPUs ought to receive and process it */

	clear_ipi_counts();
	arch_sched_broadcast_ipi();
	zassert_true(wait_for_ipis(set, CONFIG_MP_MAX_NUM_CPUS,
				  IPI_ALL_CPUS_MASK ^ BIT(id)),
		     "Timed out waiting for broadcast IPIs.\n");

	for (j = 0; j < CONFIG_MP_MAX_NUM_CPUS; j++) {
		if (id == j) {
			zassert_true(set[j] == 0,
				     "Broadcast-Expected 0, got %u\n",
				     set[j]);
		} else {
			zassert_true(set[j] == 1,
				     "Broadcast-Expected 1, got %u\n",
				     set[j]);
		}
	}
}

/* __DOXYGEN__ is predefined in the traceability build so the
 * requirement-annotated test below stays visible to Doxygen.
 */
#if defined(CONFIG_ARCH_HAS_DIRECTED_IPIS) || defined(__DOXYGEN__)
/**
 * @brief Verify that arch_sched_directed_ipi() reaches only the targeted CPU.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * On architectures with directed IPIs, an IPI sent to a single CPU mask must
 * be delivered to that CPU alone; no other CPU may take the interrupt. Each
 * CPU is targeted in turn while all of them are kept busy, and the
 * z_trace_sched_ipi() hook counts the IPIs each CPU receives.
 *
 * Test steps:
 * - Create one busy cooperative thread per other CPU and wait until all of
 *   them are running.
 * - For each CPU other than the current one: clear the IPI counts, call
 *   arch_sched_directed_ipi() with only that CPU's bit set, and wait for the
 *   IPI to be observed.
 * - Read the per-CPU counts after each directed IPI.
 *
 * Expected result:
 * - Only the targeted CPU received an IPI in each iteration; every other CPU
 *   count stayed at zero.
 *
 * @see arch_sched_directed_ipi()
 */
ZTEST(ipi, test_ipi_directed_reaches_target_cpu)
{
	uint32_t  set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t  id;
	int priority;
	unsigned int j;

	priority = k_thread_priority_get(k_current_get());

	id = busy_threads_create(priority - 1);

	/*
	 * Send an IPI to each CPU, one at a time. Verify that only the
	 * targeted CPU received the IPI.
	 */
	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (i == id) {
			continue;
		}

		clear_ipi_counts();
		arch_sched_directed_ipi(BIT(i));
		zassert_true(wait_for_ipis(set, CONFIG_MP_MAX_NUM_CPUS,
					  BIT(i)),
			     "Timed out waiting for directed IPI.\n");

		for (j = 0; j < CONFIG_MP_MAX_NUM_CPUS; j++) {
			if (i == j) {
				zassert_true(set[j] == 1,
					     "Direct-Expected 1, got %u\n",
					     set[j]);
			} else {
				zassert_true(set[j] == 0,
					     "Direct-Expected 0, got %u\n",
					     set[j]);
			}
		}
	}
}
#endif

/**
 * @brief Verify that waking a low priority thread sends no IPIs.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * With IPI optimization enabled, the scheduler only signals a CPU when the
 * newly ready thread could actually preempt what that CPU is running. A thread
 * whose priority is lower than every currently executing thread cannot preempt
 * anything, so waking it must not generate any IPI at all.
 *
 * Test steps:
 * - Create a low priority thread and wait until it pends on a semaphore.
 * - Create busy threads on all other CPUs, then lower their priority above the
 *   pending thread's.
 * - Clear the per-CPU IPI counts and give the semaphore to wake the low
 *   priority thread.
 * - Busy-wait for any IPIs to be processed, then read all counts.
 *
 * Expected result:
 * - The woken thread becomes ready and no CPU received an IPI.
 *
 * @see k_sem_give()
 * @see z_is_thread_ready()
 */
ZTEST(ipi, test_ipi_low_thread_wake_sends_none)
{
	uint32_t  set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t  id;
	int priority;
	unsigned int i;

	priority = k_thread_priority_get(k_current_get());
	atomic_clear(&busy_started);

	alt_thread_create(5, "Low");

	id = busy_threads_create(priority - 1);

	/*
	 * Lower the priority of the busy threads now that we know that they
	 * have started. As this is expected to generate IPIs, busy wait for
	 * some small amount of time to give them time to be processed.
	 */

	busy_threads_priority_set(0, 0);
	k_busy_wait(DELAY_FOR_IPIS);

	/*
	 * Low priority thread is pended. Current thread is cooperative.
	 * Other CPUs are executing preemptible threads @ priority 0.
	 */

	clear_ipi_counts();
	k_sem_give(&sem);
	k_busy_wait(DELAY_FOR_IPIS);
	get_ipi_counts(set, CONFIG_MP_MAX_NUM_CPUS);

	zassert_true(z_is_thread_ready(&alt_thread),
		     "Low priority thread is not ready.\n");

	alt_thread_done = true;

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		zassert_true(set[i] == 0,
			     "CPU %u unexpectedly received IPI.\n", i);
	}
}

/**
 * @brief Verify that waking a high priority thread sends the expected IPIs.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * The counterpart of the low priority case: a thread that outranks every
 * executing thread must preempt one of them, so the scheduler has to signal
 * the other CPUs. The current CPU is running a cooperative thread and must
 * never signal itself.
 *
 * Test steps:
 * - Create a high priority thread and wait until it pends on a semaphore.
 * - Create busy threads on all other CPUs, then lower their priorities.
 * - Clear the per-CPU IPI counts and give the semaphore to wake the high
 *   priority thread.
 * - Wait for the IPIs to be observed and read all counts.
 *
 * Expected result:
 * - The woken thread becomes ready, every other CPU received exactly one IPI,
 *   and the current CPU received none.
 *
 * @see k_sem_give()
 * @see z_is_thread_ready()
 */
ZTEST(ipi, test_ipi_high_thread_wake_sends_all)
{
	uint32_t  set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t  id;
	int priority;
	unsigned int i;

	priority = k_thread_priority_get(k_current_get());
	atomic_clear(&busy_started);

	alt_thread_create(priority - 1 - NUM_THREADS, "High");

	id = busy_threads_create(priority - 1);

	/*
	 * Lower the priority of the busy threads now that we know that they
	 * have started and are busy waiting. As this is expected to generate
	 * IPIs, busy wait for some small amount of time to give them time to
	 * be processed.
	 */

	busy_threads_priority_set(0, 1);
	k_busy_wait(DELAY_FOR_IPIS);

	/*
	 * High priority thread is pended. Current thread is cooperative.
	 * Other CPUs are executing preemptible threads.
	 */

	clear_ipi_counts();
	k_sem_give(&sem);
	zassert_true(wait_for_ipis(set, CONFIG_MP_MAX_NUM_CPUS,
				  IPI_ALL_CPUS_MASK ^ BIT(id)),
		     "Timed out waiting for wake IPIs.\n");

	zassert_true(z_is_thread_ready(&alt_thread),
		     "High priority thread is not ready.\n");

	alt_thread_done = true;

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (i == id) {
			continue;
		}

		zassert_true(set[i] == 1, "CPU%u got %u IPIs", i, set[i]);
	}

	zassert_true(set[id] == 0, "Current CPU got %u IPI(s).\n", set[id]);
}

/**
 * @brief Verify that lowering an executing thread's priority sends an IPI.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * Lowering the priority of a thread that is currently executing on another CPU
 * may make a ready thread eligible to preempt it, so that CPU must be told to
 * reschedule. Where directed IPIs are available only the CPU running the
 * demoted thread is signalled; otherwise the IPI is broadcast to every CPU but
 * the current one. The expected mask is built accordingly, so the test asserts
 * on exactly the CPUs that should have been signalled.
 *
 * Test steps:
 * - Create busy threads on all other CPUs and wait until they run.
 * - Compute the expected IPI mask: the CPU executing the target thread when
 *   directed IPIs are supported, otherwise all CPUs but the current one.
 * - Clear the per-CPU IPI counts and call k_thread_priority_set() to lower the
 *   target thread's priority.
 * - Wait for the expected IPIs and read all counts.
 *
 * Expected result:
 * - Exactly the CPUs in the expected mask received one IPI each, and the
 *   current CPU received none.
 *
 * @see k_thread_priority_set()
 */
ZTEST(ipi, test_ipi_priority_set_lower_sends)
{
	uint32_t  set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t  id;
	uint32_t  expected_mask;
	int priority;
	unsigned int i;

	priority = k_thread_priority_get(k_current_get());

	id = busy_threads_create(priority - 1);
	expected_mask = IPI_ALL_CPUS_MASK ^ BIT(id);

#ifdef CONFIG_ARCH_HAS_DIRECTED_IPIS
	expected_mask = 0U;
	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if ((i != id) && (_kernel.cpus[i].current == &thread[0])) {
			expected_mask = BIT(i);
			break;
		}
	}
	zassert_true(expected_mask != 0U,
		     "thread[0] is not executing on another CPU\n");
#endif

	clear_ipi_counts();
	k_thread_priority_set(&thread[0], priority);
	zassert_true(wait_for_ipis(set, CONFIG_MP_MAX_NUM_CPUS, expected_mask),
		     "Timed out waiting for priority change IPIs.\n");

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (i == id) {
			continue;
		}

#ifdef CONFIG_ARCH_HAS_DIRECTED_IPIS
		if ((expected_mask & BIT(i)) != 0U) {
			zassert_true(set[i] == 1, "CPU%u got %u IPIs.\n",
				     i, set[i]);
		} else {
			zassert_true(set[i] == 0, "CPU%u got %u IPI(s).\n",
				     i, set[i]);
		}
#else
		zassert_true(set[i] == 1, "CPU%u got %u IPIs", i, set[i]);
#endif
	}

	zassert_true(set[id] == 0, "Current CPU got %u IPI(s).\n", set[id]);
}

/**
 * @brief Verify that no IPIs are sent to CPUs running cooperative threads.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * A cooperative thread cannot be preempted, so signalling the CPU it runs on
 * would achieve nothing. Even waking a thread of higher priority than every
 * running thread must therefore leave the CPUs executing cooperative threads
 * untouched.
 *
 * Test steps:
 * - Create a high priority thread and wait until it pends on a semaphore.
 * - Create cooperative busy threads on all other CPUs and leave their
 *   priorities unchanged.
 * - Clear the per-CPU IPI counts and give the semaphore to wake the high
 *   priority thread.
 * - Busy-wait for any IPIs to be processed, then read all counts.
 *
 * Expected result:
 * - The woken thread becomes ready and no CPU received an IPI.
 *
 * @see k_sem_give()
 * @see K_PRIO_COOP()
 */
ZTEST(ipi, test_ipi_coop_cpus_receive_none)
{
	uint32_t  set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t  id;
	int priority;
	unsigned int i;

	priority = k_thread_priority_get(k_current_get());
	atomic_clear(&busy_started);

	alt_thread_create(priority - 1 - NUM_THREADS, "High");

	id = busy_threads_create(priority - 1);

	/*
	 * High priority thread is pended. Current thread is cooperative.
	 * Other CPUs are executing lower priority cooperative threads.
	 */

	clear_ipi_counts();
	k_sem_give(&sem);
	k_busy_wait(DELAY_FOR_IPIS);
	get_ipi_counts(set, CONFIG_MP_MAX_NUM_CPUS);

	zassert_true(z_is_thread_ready(&alt_thread),
		     "High priority thread is not ready.\n");

	alt_thread_done = true;

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		zassert_true(set[i] == 0, "CPU%u got %u IPIs", i, set[i]);
	}
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

	alt_thread_done = false;
}

ZTEST_SUITE(ipi, NULL, ipi_tests_setup, NULL, cleanup_threads, NULL);

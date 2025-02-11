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
#include <zephyr/kernel_structs.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define NUM_THREADS (CONFIG_MP_MAX_NUM_CPUS - 1)

#define DELAY_FOR_IPIS 200

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

	k_busy_wait(10000);
	zassert_true(z_is_thread_pending(&alt_thread),
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
 * Verify that arch_sched_broadcast_ipi() broadcasts IPIs as expected.
 */
ZTEST(ipi, test_arch_sched_broadcast_ipi)
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
	k_busy_wait(DELAY_FOR_IPIS);
	get_ipi_counts(set, CONFIG_MP_MAX_NUM_CPUS);

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

#ifdef CONFIG_ARCH_HAS_DIRECTED_IPIS
/**
 * Verify that arch_sched_directed_ipi() directs IPIs as expected.
 */
ZTEST(ipi, test_arch_sched_directed_ipi)
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
		k_busy_wait(DELAY_FOR_IPIS);
		get_ipi_counts(set, CONFIG_MP_MAX_NUM_CPUS);

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
 * Verify that waking a thread whose priority is lower than any other
 * currently executing thread does not result in any IPIs being sent.
 */
ZTEST(ipi, test_low_thread_wakes_no_ipis)
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
 * Verify that waking a thread whose priority is higher than all currently
 * executing threads results in the proper IPIs being sent and processed.
 */
ZTEST(ipi, test_high_thread_wakes_some_ipis)
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
	k_busy_wait(DELAY_FOR_IPIS);
	get_ipi_counts(set, CONFIG_MP_MAX_NUM_CPUS);

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
 * Verify that lowering the priority of an active thread results in an IPI.
 * If directed IPIs are enabled, then only the CPU executing that active
 * thread ought to receive the IPI. Otherwise if IPIs are broadcast, then all
 * other CPUs save the current CPU ought to receive IPIs.
 */
ZTEST(ipi, test_thread_priority_set_lower)
{
	uint32_t  set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t  id;
	int priority;
	unsigned int i;

	priority = k_thread_priority_get(k_current_get());

	id = busy_threads_create(priority - 1);

	clear_ipi_counts();
	k_thread_priority_set(&thread[0], priority);
	k_busy_wait(DELAY_FOR_IPIS);
	get_ipi_counts(set, CONFIG_MP_MAX_NUM_CPUS);

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (i == id) {
			continue;
		}

#ifdef CONFIG_ARCH_HAS_DIRECTED_IPIS
		unsigned int j;

		for (j = 0; j < NUM_THREADS; j++) {
			if (_kernel.cpus[i].current == &thread[j]) {
				break;
			}
		}

		zassert_true(j < NUM_THREADS,
			     "CPU%u not executing expected thread\n", i);

		if (j == 0) {
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

/*
 * Verify that IPIs are not sent to CPUs that are executing cooperative
 * threads.
 */
ZTEST(ipi, test_thread_coop_no_ipis)
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

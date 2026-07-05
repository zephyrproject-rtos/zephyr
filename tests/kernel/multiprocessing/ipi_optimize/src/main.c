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
#define SETUP_IPI_DRAIN_US 20000
#define WAIT_FOR_IPIS_US 100000
#define WAIT_FOR_THREAD_PENDING_US 100000

#define CPU_UNSET (-1)

static struct k_thread thread[NUM_THREADS];
static struct k_thread alt_thread;

static bool alt_thread_created;

static K_THREAD_STACK_ARRAY_DEFINE(stack, NUM_THREADS, STACK_SIZE);
static K_THREAD_STACK_DEFINE(alt_stack, STACK_SIZE);

static uint32_t ipi_count[CONFIG_MP_MAX_NUM_CPUS];
static struct k_spinlock ipilock;
static atomic_t busy_started;
static volatile bool alt_thread_done;
static atomic_t alt_started;

static atomic_t metairq_trigger;
static atomic_t metairq_initial_cpu;
static atomic_t metairq_cpu;
static atomic_t metairq_resumed_cpu;

static K_SEM_DEFINE(sem, 0, 1);
static K_SEM_DEFINE(metairq_sem, 0, 1);

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
	atomic_set(&alt_started, 1);

	while (!alt_thread_done) {
		key = arch_irq_lock();
		arch_spin_relax();
		arch_irq_unlock(key);
	}
}

static void metairq_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t id;
	int key;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_take(&metairq_sem, K_FOREVER);

	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);
	atomic_set(&metairq_cpu, (atomic_val_t)id);

	while (atomic_get(&metairq_resumed_cpu) == CPU_UNSET) {
		key = arch_irq_lock();
		arch_spin_relax();
		arch_irq_unlock(key);
	}
}

static void metairq_waker_entry(void *p1, void *p2, void *p3)
{
	uint32_t id;
	int key;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);
	atomic_set(&metairq_initial_cpu, (atomic_val_t)id);

	while (atomic_get(&metairq_trigger) == 0) {
		key = arch_irq_lock();
		arch_spin_relax();
		arch_irq_unlock(key);
	}

	k_sem_give(&metairq_sem);

	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);
	atomic_set(&metairq_resumed_cpu, (atomic_val_t)id);
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
 * Verify that a single wakeup targeting idle CPUs sends exactly one IPI.
 */
ZTEST(ipi, test_idle_thread_wakes_one_cpu)
{
	uint32_t set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t id;
	uint32_t total = 0U;
	int key;
	int priority = k_thread_priority_get(k_current_get());

	if (!IS_ENABLED(CONFIG_IPI_OPTIMIZE_IDLE)) {
		ztest_test_skip();
	}

	atomic_clear(&alt_started);
	alt_thread_create(priority + 1, "Idle");

	key = arch_irq_lock();
	id = _current_cpu->id;
	arch_irq_unlock(key);

	/* Wait for any in-flight scheduling IPIs from setup to be consumed. */
	k_busy_wait(SETUP_IPI_DRAIN_US);
	clear_ipi_counts();
	k_sem_give(&sem);
	bool started = WAIT_FOR(atomic_get(&alt_started) != 0,
				WAIT_FOR_IPIS_US, k_busy_wait(10));

	zassert_true(started, "Timed out waiting for idle target thread.\n");
	get_ipi_counts(set, CONFIG_MP_MAX_NUM_CPUS);
	alt_thread_done = true;

	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (i != id) {
			total += set[i];
		}
	}

	zassert_equal(total, 1U, "Expected one idle wake IPI, got %u", total);
	zassert_equal(set[id], 0U, "Current CPU got %u IPI(s)", set[id]);
}

/**
 * Verify that idle CPU reservations remain unique and can be rebound without
 * requesting another IPI.
 */
ZTEST(ipi, test_ipi_reservation_tracking)
{
	atomic_val_t first_mask;
	atomic_val_t second_mask;
	atomic_val_t duplicate_mask;
	atomic_val_t rebound_mask;
	bool rebound;
	int priority = k_thread_priority_get(k_current_get());

	if (!IS_ENABLED(CONFIG_IPI_OPTIMIZE_IDLE) ||
	    CONFIG_MP_MAX_NUM_CPUS < 4 ||
	    !IS_ENABLED(CONFIG_ARCH_HAS_DIRECTED_IPIS)) {
		ztest_test_skip();
	}

	for (unsigned int i = 0; i < 2; i++) {
		k_thread_create(&thread[i], stack[i], STACK_SIZE,
				pending_thread_entry, NULL, NULL, NULL,
				priority + 1, 0, K_FOREVER);
	}

	k_spinlock_key_t sched_key = k_spin_lock(&_sched_spinlock);

	first_mask = ipi_mask_create(&thread[0]);
	second_mask = ipi_mask_create(&thread[1]);
	duplicate_mask = ipi_mask_create(&thread[1]);
	ipi_idle_thread_unreserve(&thread[0]);
	rebound = ipi_idle_thread_rebind(&thread[1], &thread[0]);
	rebound_mask = ipi_mask_create(&thread[0]);
	ipi_idle_thread_unreserve(&thread[0]);
	k_spin_unlock(&_sched_spinlock, sched_key);

	zassert_not_equal(first_mask, 0, "First thread was not reserved");
	zassert_not_equal(second_mask, 0, "Second thread was not reserved");
	zassert_equal(first_mask & second_mask, 0,
		      "Threads shared reservation mask 0x%lx",
		      (long)(first_mask & second_mask));
	zassert_equal(duplicate_mask, 0,
		      "Reserved thread was duplicated on CPU mask 0x%lx",
		      (long)duplicate_mask);
	zassert_true(rebound, "Reservation was not rebound");
	zassert_equal(rebound_mask, 0,
		      "Rebound thread requested IPI mask 0x%lx", (long)rebound_mask);
}

/**
 * Verify that a MetaIRQ preempts its local waker and sends one IPI so the
 * displaced waker can resume on an idle CPU.
 */
ZTEST(ipi, test_metairq_wakes_displaced_thread_on_idle_cpu)
{
	uint32_t set[CONFIG_MP_MAX_NUM_CPUS];
	uint32_t initial_cpu;
	uint32_t ipi_total = 0U;
	int priority = k_thread_priority_get(k_current_get());

	if (!IS_ENABLED(CONFIG_IPI_OPTIMIZE_IDLE) ||
	    CONFIG_NUM_METAIRQ_PRIORITIES == 0 ||
	    CONFIG_MP_MAX_NUM_CPUS < 3 ||
	    !IS_ENABLED(CONFIG_ARCH_HAS_DIRECTED_IPIS)) {
		ztest_test_skip();
	}

	atomic_clear(&metairq_trigger);
	atomic_set(&metairq_initial_cpu, CPU_UNSET);
	atomic_set(&metairq_cpu, CPU_UNSET);
	atomic_set(&metairq_resumed_cpu, CPU_UNSET);
	k_sem_reset(&metairq_sem);

	k_thread_create(&alt_thread, alt_stack, STACK_SIZE,
			metairq_thread_entry, NULL, NULL, NULL,
			K_HIGHEST_THREAD_PRIO, 0, K_FOREVER);
	alt_thread_created = true;
	k_thread_start(&alt_thread);
	zassert_true(WAIT_FOR(z_is_thread_pending(&alt_thread),
			      WAIT_FOR_THREAD_PENDING_US, k_busy_wait(10)),
		     "MetaIRQ thread did not pend");

	k_thread_create(&thread[0], stack[0], STACK_SIZE,
			metairq_waker_entry, NULL, NULL, NULL,
			priority + 1, 0, K_NO_WAIT);
	zassert_true(WAIT_FOR(atomic_get(&metairq_initial_cpu) != CPU_UNSET,
			      WAIT_FOR_THREAD_PENDING_US, k_busy_wait(10)),
		     "MetaIRQ waker did not start");

	k_busy_wait(SETUP_IPI_DRAIN_US);
	clear_ipi_counts();
	atomic_set(&metairq_trigger, 1);

	zassert_true(WAIT_FOR(atomic_get(&metairq_cpu) != CPU_UNSET,
			      WAIT_FOR_IPIS_US, k_busy_wait(10)),
		     "MetaIRQ thread did not run");
	zassert_true(WAIT_FOR(atomic_get(&metairq_resumed_cpu) != CPU_UNSET,
			      WAIT_FOR_IPIS_US, k_busy_wait(10)),
		     "Displaced waker did not resume");

	initial_cpu = (uint32_t)atomic_get(&metairq_initial_cpu);
	zassert_equal((uint32_t)atomic_get(&metairq_cpu), initial_cpu,
		      "MetaIRQ did not stay on CPU%u", initial_cpu);
	zassert_not_equal((uint32_t)atomic_get(&metairq_resumed_cpu), initial_cpu,
			  "Displaced waker stayed on CPU%u", initial_cpu);

	get_ipi_counts(set, CONFIG_MP_MAX_NUM_CPUS);
	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		ipi_total += set[i];
	}
	zassert_equal(ipi_total, 1U, "Expected one idle IPI, got %u", ipi_total);
}

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
 * Verify that lowering the priority of an active thread results in an IPI.
 * If directed IPIs are enabled, then only the CPU executing that active
 * thread ought to receive the IPI. Otherwise if IPIs are broadcast, then all
 * other CPUs save the current CPU ought to receive IPIs.
 */
ZTEST(ipi, test_thread_priority_set_lower)
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

	atomic_clear(&metairq_trigger);
	atomic_set(&metairq_initial_cpu, CPU_UNSET);
	atomic_set(&metairq_cpu, CPU_UNSET);
	atomic_set(&metairq_resumed_cpu, CPU_UNSET);
	alt_thread_done = false;
	atomic_clear(&alt_started);
}

ZTEST_SUITE(ipi, NULL, ipi_tests_setup, NULL, cleanup_threads, NULL);

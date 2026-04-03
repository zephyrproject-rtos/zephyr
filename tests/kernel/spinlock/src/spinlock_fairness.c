/*
 * Copyright (c) 2023 Syntacore. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/ztest_test.h"
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/spinlock.h>

#ifdef CONFIG_SCHED_CPU_MASK

#define STACK_SIZE	(8 * 1024)
#define CORES_NUM	CONFIG_MP_MAX_NUM_CPUS
#define FAIRNESS_TEST_CYCLES_PER_CORE 1000

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS > 1);

static K_THREAD_STACK_ARRAY_DEFINE(tstack, CORES_NUM, STACK_SIZE);
static struct k_thread tthread[CORES_NUM];
static uint32_t spinlock_grabbed[CORES_NUM];
static atomic_t fairness_test_cycles;
static struct k_spinlock lock;
static atomic_t start_sync;

static inline struct k_thread *get_thread(uint8_t core_id)
{
	return &tthread[core_id];
}

/**
 * @brief Execution thread which runs concurrently on each CPU in the system
 *
 * @param [in] arg1 - thread argument 1
 * @param [in] arg2 - thread argument 2
 * @param [in] arg3 - thread argument 3
 */
static void test_thread(void *arg1, void *arg2, void *arg3)
{
	int core_id = (uintptr_t)arg1;

	/* Synchronize all the cores as much as possible */
	int key = arch_irq_lock();

	atomic_dec(&start_sync);
	while (atomic_get(&start_sync) != 0)
		;

	/*
	 * Run the test: let the cores contend for the spinlock and
	 * collect the spinlock acquisition statistics
	 */
	do {
		k_spinlock_key_t spinlock_key = k_spin_lock(&lock);

		if (atomic_get(&fairness_test_cycles) == 0) {
			k_spin_unlock(&lock, spinlock_key);
			arch_irq_unlock(key);
			return;
		}

		spinlock_grabbed[core_id]++;

		/* Imitate some work which takes time */
		volatile uint32_t countdown = 10000;

		while (countdown--)
			;

		atomic_dec(&fairness_test_cycles);

		k_spin_unlock(&lock, spinlock_key);
	} while (atomic_get(&fairness_test_cycles) != 0);

	arch_irq_unlock(key);
}

static void test_init(void)
{
	memset(tthread, 0x00, sizeof(tthread));
	memset(tstack, 0x00, sizeof(tstack));
	atomic_set(&start_sync, CORES_NUM);
	atomic_set(&fairness_test_cycles, FAIRNESS_TEST_CYCLES_PER_CORE * CORES_NUM);

	for (uintptr_t core_id = 0; core_id < CORES_NUM; core_id++) {
		struct k_thread *thread = get_thread(core_id);

		k_thread_create(thread, tstack[core_id], STACK_SIZE,
		   (k_thread_entry_t)test_thread,
		   (void *)core_id, NULL, NULL,
		   K_PRIO_COOP(10), 0, K_FOREVER);

		/*
		 * Pin each thread to a particular CPU core.
		 * The larger the core's memory access latency in comparison to the
		 * other cores - the less chances to win a contention for the spinlock
		 * this core will have in case the spinlock implementation doesn't
		 * provide acquisition fairness.
		 */
		k_thread_cpu_pin(thread, core_id);
	}
}

/**
 * @brief Test spinlock acquisition fairness
 *
 * @details This test verifies a spinlock acquisition fairness in relation
 *		to the cores contending for the spinlock. Memory access latency may
 *		vary between the CPU cores, so that some CPUs reach the spinlock faster
 *		than the others and depending on spinlock implementation may get
 *		higher chance to win the contention for the spinlock than the other
 *		cores, making them to starve.
 *		This effect may become critical for some real-life platforms
 *		(e.g. NUMA) resulting in performance loss or even a live-lock,
 *		when a single CPU is continuously winning the contention.
 *		This test ensures that the probability to win the contention for a
 *		spinlock is evenly distributed between all of the contending cores.
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see k_spin_lock(), k_spin_unlock()
 */
ZTEST(spinlock, test_spinlock_fairness)
{
	test_init();

	/* Launching all the threads */
	for (uint8_t core_id = 0; core_id < CORES_NUM; core_id++) {
		struct k_thread *thread = get_thread(core_id);

		k_thread_start(thread);
	}
	/* Waiting for all the threads to complete */
	for (uint8_t core_id = 0; core_id < CORES_NUM; core_id++) {
		struct k_thread *thread = get_thread(core_id);

		k_thread_join(thread, K_FOREVER);
	}

	/* Print statistics */
	for (uint8_t core_id = 0; core_id < CORES_NUM; core_id++) {
		printk("CPU%u acquired spinlock %u times, expected %u\n",
			core_id, spinlock_grabbed[core_id], FAIRNESS_TEST_CYCLES_PER_CORE);
	}

	/* Verify spinlock acquisition fairness */
	for (uint8_t core_id = 0; core_id < CORES_NUM; core_id++) {
		zassert_false(spinlock_grabbed[core_id] < FAIRNESS_TEST_CYCLES_PER_CORE,
			"CPU%d starved on a spinlock: acquired %u times, expected %u\n",
			core_id, spinlock_grabbed[core_id], FAIRNESS_TEST_CYCLES_PER_CORE);
	}
}

#endif /* CONFIG_SCHED_CPU_MASK */

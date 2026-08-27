/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/cpu_freq.h>

#define NUM_THREADS (2 * (CONFIG_MP_MAX_NUM_CPUS - 1))

LOG_MODULE_REGISTER(cpu_freq_soc_test, LOG_LEVEL_INF);

const struct pstate *soc_pstates_dt[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))};

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
static K_THREAD_STACK_ARRAY_DEFINE(busy_thread_stacks, NUM_THREADS, 1024);
static struct k_thread busy_threads[NUM_THREADS];
static struct k_sem busy_thread_sem[NUM_THREADS];

/**
 * For each extra CPU, two threads are created that ping-pong giving and taking
 * semaphores. Not only does this keep the CPUs busy, it generates scheduling
 * point IPIs which can be used to validate a test environment assumption--
 * that the current schedule lock will be respected.
 */
static void give_take_helper(void *p1, void *p2, void *p3)
{
	int order = (int)(intptr_t)p1;
	struct k_sem *sem1 = (struct k_sem *)p2;
	struct k_sem *sem2 = (struct k_sem *)p3;

	while (1) {
		if ((order & 1) == 0) {
			k_sem_give(sem1);
			k_sem_take(sem2, K_FOREVER);
		} else {
			k_sem_give(sem2);
			k_sem_take(sem1, K_FOREVER);
		}
	}
}
#endif

/*
 * Test SoC integration of CPU Freq
 */
ZTEST(cpu_freq_soc, test_soc_pstates)
{
	int ret;
	int i;

	zassert_true(ARRAY_SIZE(soc_pstates_dt) > 0, "No P-states defined in devicetree");

	LOG_INF("%zu P-states defined for %s", ARRAY_SIZE(soc_pstates_dt), CONFIG_BOARD_TARGET);

	zassert_equal(cpu_freq_pstate_set(NULL), -EINVAL, "Expected -EINVAL for NULL pstate");

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	int priority;
	unsigned int id1;
	unsigned int id2;

	priority = k_thread_priority_get(k_current_get());

	for (i = 0; i < NUM_THREADS; i++) {
		k_sem_init(&busy_thread_sem[i], 0, 1);
	}

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&busy_threads[i], busy_thread_stacks[i],
				K_THREAD_STACK_SIZEOF(busy_thread_stacks[i]),
				(k_thread_entry_t)give_take_helper,
				(void *)(intptr_t)i,
				&busy_thread_sem[i / 2],
				&busy_thread_sem[1 + (i / 2)],
				priority, 0, K_NO_WAIT);
	}
#endif

	for (i = 0; i < ARRAY_SIZE(soc_pstates_dt); i++) {
		const struct pstate *state = soc_pstates_dt[i];

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
		/*
		 * Lock the scheduler to ensure that the current thread
		 * does not migrate to another CPU.
		 */
		k_sched_lock();

		id1 = arch_curr_cpu()->id;

		/*
		 * Validate the assumption that the current thread does not
		 * migrate across CPUs before calling cpu_freq_pstate_set().
		 */

		for (int j = 0; j < 10; j++) {
			k_busy_wait(10000);
			id2 = arch_curr_cpu()->id;
			zassert_equal(id1, id2,
				      "Current CPU changed while scheduler locked");
		}
#endif

		/* Set performance state using pstate driver */
		ret = cpu_freq_pstate_set(state);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
		/*
		 * Validate the assumption that the current thread does not
		 * migrate after calling cpu_freq_pstate_set().
		 */
		for (int j = 0; j < 10; j++) {
			k_busy_wait(10000);
			id2 = arch_curr_cpu()->id;
			zassert_equal(id1, id2,
				      "Current CPU changed while scheduler locked");
		}

		k_sched_unlock();
#endif
		zassert_equal(ret, 0, "Failed to set P-state %d", i);
	}

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(&busy_threads[i]);
	}
#endif
}

ZTEST_SUITE(cpu_freq_soc, NULL, NULL, NULL, NULL, NULL);

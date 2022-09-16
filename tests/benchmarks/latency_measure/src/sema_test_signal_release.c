/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file measure time for sema lock and release
 *
 * This file contains the test that measures semaphore and mutex lock and
 * release time in the kernel. There is no contention on the sema nor the
 * mutex being tested.
 */

#include <zephyr/zephyr.h>
#include <zephyr/timing/timing.h>
#include "utils.h"

/* the number of semaphore give/take cycles */
#define N_TEST_SEMA 1000

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
/* stack used by the threads */
static K_THREAD_STACK_DEFINE(thread_one_stack, STACK_SIZE);

static struct k_thread thread_one_data;

K_SEM_DEFINE(lock_unlock_sema, 0, N_TEST_SEMA);

K_SEM_DEFINE(sem_bench, 0, 1);

timing_t timestamp_start_sema_t_c;
timing_t timestamp_end_sema_t_c;
timing_t timestamp_start_sema_g_c;
timing_t timestamp_end_sema_g_c;

void thread_sema_test1(void *p1, void *p2, void *p3)
{
	timestamp_start_sema_t_c = timing_counter_get();
	k_sem_take(&sem_bench, K_FOREVER);
	timestamp_end_sema_g_c = timing_counter_get();
}

int sema_context_switch(void)
{
	uint32_t diff;

	bench_test_start();
	timing_start();

	k_thread_create(&thread_one_data, thread_one_stack,
			STACK_SIZE, thread_sema_test1,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(3), 0, K_FOREVER);
	k_thread_name_set(&thread_one_data, "sema_test1");
	k_thread_start(&thread_one_data);

	timestamp_end_sema_t_c = timing_counter_get();
	diff = timing_cycles_get(&timestamp_start_sema_t_c, &timestamp_end_sema_t_c);
	PRINT_STATS("Semaphore take time (context switch)", diff);


	timestamp_start_sema_g_c = timing_counter_get();
	k_sem_give(&sem_bench);
	diff = timing_cycles_get(&timestamp_start_sema_g_c, &timestamp_end_sema_g_c);
	PRINT_STATS("Semaphore give time (context switch)", diff);

	timing_stop();

	return 0;
}

/**
 *
 * @brief The function tests semaphore test/signal time
 *
 * The routine performs unlock the quite amount of semaphores and then
 * acquires them in order to measure the necessary time.
 *
 * @return 0 on success
 */
int sema_test_signal(void)
{
	int i;
	uint32_t diff;
	timing_t timestamp_start;
	timing_t timestamp_end;

	bench_test_start();
	timing_start();

	timestamp_start = timing_counter_get();

	for (i = 0; i < N_TEST_SEMA; i++) {
		k_sem_give(&lock_unlock_sema);
	}

	timestamp_end = timing_counter_get();
	timing_stop();

	if (bench_test_end() == 0) {
		diff = timing_cycles_get(&timestamp_start, &timestamp_end);
		PRINT_STATS_AVG("Average semaphore signal time", diff, N_TEST_SEMA);
	} else {
		error_count++;
		PRINT_OVERFLOW_ERROR();
	}

	bench_test_start();
	timing_start();

	timestamp_start = timing_counter_get();

	for (i = 0; i < N_TEST_SEMA; i++) {
		k_sem_take(&lock_unlock_sema, K_FOREVER);
	}

	timestamp_end = timing_counter_get();
	timing_stop();

	if (bench_test_end() == 0) {
		diff = timing_cycles_get(&timestamp_start, &timestamp_end);
		PRINT_STATS_AVG("Average semaphore test time", diff, N_TEST_SEMA);
	} else {
		error_count++;
		PRINT_OVERFLOW_ERROR();
	}

	return 0;
}

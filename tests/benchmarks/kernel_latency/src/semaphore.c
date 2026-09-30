/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Semaphore operations. The uncontended give and take are timed by the
 * framework; waking a waiting thread is measured manually, because the
 * span ends in that thread.
 */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(semaphore, NULL, NULL);

static K_SEM_DEFINE(sem, 0, 1);

static void sem_drain(void)
{
	(void)k_sem_take(&sem, K_NO_WAIT);
}

static void sem_fill(void)
{
	k_sem_give(&sem);
}

ZTEST_BENCHMARK(semaphore, give, BENCH_SAMPLES, NULL, sem_drain)
{
	k_sem_give(&sem);
}

ZTEST_BENCHMARK(semaphore, take, BENCH_SAMPLES, sem_fill, NULL)
{
	(void)k_sem_take(&sem, K_NO_WAIT);
}

/*
 * Time from k_sem_give() to the higher priority thread that was
 * blocked on the semaphore running again: the wake plus the context
 * switch it forces.
 */
static K_SEM_DEFINE(wake_sem, 0, 1);
static K_SEM_DEFINE(done_sem, 0, 1);
static K_THREAD_STACK_DEFINE(waiter_stack, BENCH_PARTNER_STACK_SIZE);
static struct k_thread waiter_thread;
static volatile timing_t wake_start;
static volatile timing_t wake_finish;

static void waiter_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)k_sem_take(&wake_sem, K_FOREVER);
	wake_finish = timing_counter_get();
	k_sem_give(&done_sem);
}

static void waiter_create(void)
{
	int priority = k_thread_priority_get(k_current_get()) - 1;

	k_thread_create(&waiter_thread, waiter_stack, BENCH_PARTNER_STACK_SIZE, waiter_entry,
			NULL, NULL, NULL, priority, 0, K_NO_WAIT);
}

static void waiter_join(void)
{
	(void)k_thread_join(&waiter_thread, K_FOREVER);
}

ZTEST_BENCHMARK_MANUAL(semaphore, give_wake_switch, BENCH_SAMPLES, waiter_create, waiter_join)
{
	wake_start = timing_counter_get();
	k_sem_give(&wake_sem);
	(void)k_sem_take(&done_sem, K_FOREVER);

	bench_span(wake_start, wake_finish);
}

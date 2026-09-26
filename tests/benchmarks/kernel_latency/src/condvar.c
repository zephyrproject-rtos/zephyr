/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Condition variable signalling. There is no uncontended variant worth
 * timing: signalling with nobody waiting does almost nothing, so the
 * measurement is the wake of a waiting thread.
 *
 * The span is necessarily larger than the equivalent semaphore or FIFO
 * wake. k_condvar_wait() cannot return without the mutex, so the
 * waiter only reaches its timestamp after the signaller has released
 * it, and the measurement therefore contains the mutex handoff as well
 * as the wake. That is inherent to the primitive rather than an
 * artefact of the benchmark.
 */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(condvar, NULL, NULL);

static K_MUTEX_DEFINE(mutex);
static K_CONDVAR_DEFINE(condvar);
static K_SEM_DEFINE(ready_sem, 0, 1);
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

	(void)k_mutex_lock(&mutex, K_FOREVER);
	k_sem_give(&ready_sem);
	(void)k_condvar_wait(&condvar, &mutex, K_FOREVER);

	wake_finish = timing_counter_get();

	(void)k_mutex_unlock(&mutex);
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

ZTEST_BENCHMARK_MANUAL(condvar, signal_wake_switch, BENCH_SAMPLES, waiter_create, waiter_join)
{
	/*
	 * Wait until the partner is inside k_condvar_wait() and has
	 * released the mutex, so that the signal always has somebody to
	 * wake.
	 */
	(void)k_sem_take(&ready_sem, K_FOREVER);
	(void)k_mutex_lock(&mutex, K_FOREVER);

	wake_start = timing_counter_get();
	(void)k_condvar_signal(&condvar);

	(void)k_mutex_unlock(&mutex);
	(void)k_sem_take(&done_sem, K_FOREVER);

	bench_span(wake_start, wake_finish);
}

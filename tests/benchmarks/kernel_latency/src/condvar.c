/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Condition variable signalling. There is no uncontended variant worth
 * timing: signalling with nobody waiting does almost nothing, so the
 * measurement is the wake of a waiting thread.
 *
 * k_condvar_signal() does not require the mutex, and k_condvar_wait()
 * cannot return without it, so the signaller releases the mutex before
 * opening the span. The woken waiter then takes the free mutex on its
 * way out, and the span holds one wake and one switch, like the other
 * *_wake_switch benchmarks. Signalling with the mutex held would cost
 * three switches instead: the waiter runs, blocks on the mutex and
 * boosts the signaller, which switches back, unlocks and hands over.
 */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(condvar, NULL, NULL);

static K_MUTEX_DEFINE(mutex);
static K_CONDVAR_DEFINE(condvar);
static K_SEM_DEFINE(ready_sem, 0, 1);
static K_SEM_DEFINE(done_sem, 0, 1);
static K_THREAD_STACK_DEFINE(waiter_stack, BENCH_PARTNER_STACK_SIZE);
static struct k_thread waiter_thread;

static void waiter_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)k_mutex_lock(&mutex, K_FOREVER);
	k_sem_give(&ready_sem);
	(void)k_condvar_wait(&condvar, &mutex, K_FOREVER);

	ztest_benchmark_end();

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
	 * The waiter holds the mutex from before it gives ready_sem until
	 * k_condvar_wait() has put it to sleep, so taking the mutex here
	 * guarantees that the signal has somebody to wake. It is released
	 * again before the span opens.
	 */
	(void)k_sem_take(&ready_sem, K_FOREVER);
	(void)k_mutex_lock(&mutex, K_FOREVER);
	(void)k_mutex_unlock(&mutex);

	ztest_benchmark_start();
	(void)k_condvar_signal(&condvar);

	(void)k_sem_take(&done_sem, K_FOREVER);
}

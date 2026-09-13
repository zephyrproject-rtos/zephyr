/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* k_event post and wait, and the wake of a thread blocked in wait. */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(events, NULL, NULL);

#define EVENT_BIT BIT(0)

static K_EVENT_DEFINE(event);

static void event_clear(void)
{
	k_event_clear(&event, EVENT_BIT);
}

static void event_set(void)
{
	k_event_post(&event, EVENT_BIT);
}

ZTEST_BENCHMARK(events, post, BENCH_SAMPLES, NULL, event_clear)
{
	k_event_post(&event, EVENT_BIT);
}

ZTEST_BENCHMARK(events, wait_satisfied, BENCH_SAMPLES, event_set, event_clear)
{
	(void)k_event_wait(&event, EVENT_BIT, false, K_NO_WAIT);
}

/*
 * Time from k_event_post() to the higher priority thread blocked in
 * k_event_wait() running.
 */
static K_EVENT_DEFINE(wake_event);
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

	(void)k_event_wait(&wake_event, EVENT_BIT, false, K_FOREVER);
	wake_finish = timing_counter_get();
	k_event_clear(&wake_event, EVENT_BIT);
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

ZTEST_BENCHMARK_MANUAL(events, post_wake_switch, BENCH_SAMPLES, waiter_create, waiter_join)
{
	wake_start = timing_counter_get();
	k_event_post(&wake_event, EVENT_BIT);
	(void)k_sem_take(&done_sem, K_FOREVER);

	bench_span(wake_start, wake_finish);
}

/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * FIFO operations, including the allocating variants that take memory
 * from the calling thread's resource pool, and the wake of a thread
 * blocked in k_fifo_get().
 */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(fifo, NULL, NULL);

static K_FIFO_DEFINE(fifo);

static struct {
	intptr_t reserved;
	uint32_t payload;
} item;

static void fifo_fill(void)
{
	k_fifo_put(&fifo, &item);
}

static void fifo_drain(void)
{
	(void)k_fifo_get(&fifo, K_NO_WAIT);
}

ZTEST_BENCHMARK(fifo, put, BENCH_SAMPLES, NULL, fifo_drain)
{
	k_fifo_put(&fifo, &item);
}

ZTEST_BENCHMARK(fifo, get, BENCH_SAMPLES, fifo_fill, NULL)
{
	(void)k_fifo_get(&fifo, K_NO_WAIT);
}

static void fifo_alloc_fill(void)
{
	(void)k_fifo_alloc_put(&fifo, &item);
}

ZTEST_BENCHMARK(fifo, alloc_put, BENCH_SAMPLES, NULL, fifo_drain)
{
	(void)k_fifo_alloc_put(&fifo, &item);
}

ZTEST_BENCHMARK(fifo, get_after_alloc_put, BENCH_SAMPLES, fifo_alloc_fill, NULL)
{
	(void)k_fifo_get(&fifo, K_NO_WAIT);
}

/*
 * Time from k_fifo_put() to the higher priority thread blocked in
 * k_fifo_get() running with the item.
 */
static K_FIFO_DEFINE(wake_fifo);
static K_SEM_DEFINE(done_sem, 0, 1);
static K_THREAD_STACK_DEFINE(waiter_stack, BENCH_PARTNER_STACK_SIZE);
static struct k_thread waiter_thread;
static volatile timing_t wake_start;
static volatile timing_t wake_finish;

static struct {
	intptr_t reserved;
	uint32_t payload;
} wake_item;

static void waiter_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)k_fifo_get(&wake_fifo, K_FOREVER);
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

ZTEST_BENCHMARK_MANUAL(fifo, put_wake_switch, BENCH_SAMPLES, waiter_create, waiter_join)
{
	wake_start = timing_counter_get();
	k_fifo_put(&wake_fifo, &wake_item);
	(void)k_sem_take(&done_sem, K_FOREVER);

	bench_span(wake_start, wake_finish);
}

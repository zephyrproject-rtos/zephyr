/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#define IPI_TEST_INTERVAL_MS 5000
#define WORKER_STACK_SIZE 4096

static K_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK_SIZE);
static struct k_thread worker_thread;

static K_SEM_DEFINE(wake_sem, 0, 1);
static K_SEM_DEFINE(done_sem, 0, 1);
static atomic_t ipi_counter;
static volatile uint32_t wake_cycle;
static volatile uint64_t latency_cycles;
static volatile uint32_t latency_max;
static volatile uint32_t wake_count;

void z_trace_sched_ipi(void)
{
	atomic_inc(&ipi_counter);
}

static void worker_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		k_sem_take(&wake_sem, K_FOREVER);

		uint32_t latency = k_cycle_get_32() - wake_cycle;

		latency_cycles += latency;
		latency_max = MAX(latency_max, latency);
		wake_count++;
		k_sem_give(&done_sem);
	}
}

int main(void)
{
	int priority = k_thread_priority_get(k_current_get());
	uint32_t elapsed_ms = 0U;

	k_thread_create(&worker_thread, worker_stack,
			WORKER_STACK_SIZE, worker_entry,
			NULL, NULL, NULL, priority + 1, 0, K_NO_WAIT);

	/* Let the worker block and all otherwise unused CPUs enter idle. */
	k_sleep(K_MSEC(20));

	for (;;) {
		int64_t end = k_uptime_get() + IPI_TEST_INTERVAL_MS;

		atomic_clear(&ipi_counter);
		latency_cycles = 0U;
		latency_max = 0U;
		wake_count = 0U;

		while (k_uptime_get() < end) {
			wake_cycle = k_cycle_get_32();
			k_sem_give(&wake_sem);
			k_sem_take(&done_sem, K_FOREVER);
		}

		uint32_t count = wake_count;
		uint32_t ipis = (uint32_t)atomic_get(&ipi_counter);
		uint64_t average = count == 0U ? 0U : latency_cycles / count;

		elapsed_ms += IPI_TEST_INTERVAL_MS;
		printf("**** IPI-Metric Idle Wake Test **** Elapsed Time: %u ms\n",
		       elapsed_ms);
		printf("  Wake Round Trips: %u\n", count);
		printf("  IPI Count: %u\n", ipis);
		printf("  Average Wake Latency Cycles: %llu\n",
		       (unsigned long long)average);
		printf("  Maximum Wake Latency Cycles: %u\n", latency_max);
	}
}

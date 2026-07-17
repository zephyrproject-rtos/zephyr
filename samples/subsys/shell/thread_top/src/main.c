/*
 * Copyright (c) 2026 Nerijus Bendžiūnas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define LOAD_THREAD_STACK_SIZE 1024
#define LOAD_PERIOD_MS         100

/*
 * Spin until this thread has consumed the given number of milliseconds of
 * CPU time, measured with its own runtime stats rather than wall-clock
 * time. A wall-clock busy-wait would under-deliver whenever a higher
 * priority load thread preempts this one, and the displayed percentages
 * would no longer match the thread names.
 */
static void burn_cpu_ms(uint32_t budget_ms)
{
	k_thread_runtime_stats_t start_stats;
	k_thread_runtime_stats_t now_stats;
	uint64_t budget_cycles;

	/*
	 * Thread stats count the same cycle source as sys_clock_hw_cycles
	 * because this sample keeps THREAD_RUNTIME_STATS_USE_TIMING_FUNCTIONS
	 * disabled.
	 */
	budget_cycles = ((uint64_t)sys_clock_hw_cycles_per_sec() * budget_ms) / MSEC_PER_SEC;

	k_thread_runtime_stats_get(k_current_get(), &start_stats);
	do {
		k_busy_wait(500);
		k_thread_runtime_stats_get(k_current_get(), &now_stats);
	} while ((now_stats.execution_cycles - start_stats.execution_cycles) < budget_cycles);
}

static void duty_cycle_thread(void *percent_ptr, void *unused1, void *unused2)
{
	uint32_t busy_percent = POINTER_TO_UINT(percent_ptr);
	uint32_t budget_ms = (LOAD_PERIOD_MS * busy_percent) / 100U;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (true) {
		int64_t period_start = k_uptime_get();
		int64_t elapsed;

		burn_cpu_ms(budget_ms);

		elapsed = k_uptime_get() - period_start;
		if (elapsed < LOAD_PERIOD_MS) {
			k_msleep(LOAD_PERIOD_MS - elapsed);
		}
	}
}

static void greedy_thread(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	/* Never sleeps: soaks up whatever CPU the other threads leave over. */
	while (true) {
		k_busy_wait(1000);
	}
}

K_THREAD_DEFINE(load_50_percent, LOAD_THREAD_STACK_SIZE, duty_cycle_thread, UINT_TO_POINTER(50),
		NULL, NULL, 10, 0, 0);
K_THREAD_DEFINE(load_25_percent, LOAD_THREAD_STACK_SIZE, duty_cycle_thread, UINT_TO_POINTER(25),
		NULL, NULL, 11, 0, 0);
K_THREAD_DEFINE(load_remainder, LOAD_THREAD_STACK_SIZE, greedy_thread, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

int main(void)
{
	printk("Synthetic load threads running:\n");
	printk("  load_50_percent (prio 10) burns 50%% CPU\n");
	printk("  load_25_percent (prio 11) burns 25%% CPU\n");
	printk("  load_remainder  (prio %d) spins for the rest\n",
	       K_LOWEST_APPLICATION_THREAD_PRIO);
	printk("Watch them with: kernel thread top\n");

	return 0;
}

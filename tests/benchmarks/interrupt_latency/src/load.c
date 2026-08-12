/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Background system load sources, see load.h.
 *
 * Which source perturbs which measurement depends on the context the
 * load runs in:
 *
 * - The cache load runs in the benchmark thread itself, between
 *   samples, and affects every scenario on every platform.
 * - The timer load runs in the system clock ISR, so it competes with
 *   the benchmark interrupt for the interrupt controller and for
 *   interrupt-disabled windows in the kernel. It affects every
 *   platform.
 * - The thread load runs in threads of lower priority than the
 *   benchmark. On SMP those threads execute on the other CPUs and
 *   contend for the memory system throughout. On a uniprocessor they
 *   only run while the benchmark blocks, which happens in the
 *   rescheduling scenario, so their effect there is limited.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "load.h"

#ifdef CONFIG_INT_BENCH_LOAD_CACHE
#define WORKING_SET_SIZE (CONFIG_INT_BENCH_LOAD_CACHE_KB * 1024)

/*
 * Stride of one cache line on the widest line size in common use, so
 * that every line of the working set is touched exactly once.
 */
#define WORKING_SET_STRIDE 64

static volatile uint8_t working_set[WORKING_SET_SIZE];
#endif /* CONFIG_INT_BENCH_LOAD_CACHE */

#ifdef CONFIG_INT_BENCH_LOAD_TIMER
static void load_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	/*
	 * Runs in the system clock ISR. Kept short on purpose: the
	 * point is to compete for interrupt service, not to monopolise
	 * the CPU.
	 */
}

static K_TIMER_DEFINE(load_timer, load_timer_handler, NULL);
#endif /* CONFIG_INT_BENCH_LOAD_TIMER */

#ifdef CONFIG_INT_BENCH_LOAD_THREADS
#define LOAD_STACK_SIZE  (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LOAD_BUFFER_SIZE 256

static K_THREAD_STACK_ARRAY_DEFINE(load_stacks, CONFIG_INT_BENCH_LOAD_NUM_THREADS,
				   LOAD_STACK_SIZE);
static struct k_thread load_threads[CONFIG_INT_BENCH_LOAD_NUM_THREADS];
static volatile uint32_t load_buffers[CONFIG_INT_BENCH_LOAD_NUM_THREADS][LOAD_BUFFER_SIZE];
static volatile bool load_running;

static void load_thread_entry(void *p1, void *p2, void *p3)
{
	volatile uint32_t *buffer = p1;
	uint32_t value = 0U;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (load_running) {
		for (uint32_t i = 0U; i < LOAD_BUFFER_SIZE; i++) {
			buffer[i] += value;
			value = buffer[i];
		}

		k_yield();
	}
}
#endif /* CONFIG_INT_BENCH_LOAD_THREADS */

void bench_load_start(void)
{
#ifdef CONFIG_INT_BENCH_LOAD_TIMER
	k_timer_start(&load_timer, K_USEC(CONFIG_INT_BENCH_LOAD_TIMER_PERIOD_US),
		      K_USEC(CONFIG_INT_BENCH_LOAD_TIMER_PERIOD_US));
#endif

#ifdef CONFIG_INT_BENCH_LOAD_THREADS
	int priority = k_thread_priority_get(k_current_get());

	load_running = true;

	for (uint32_t i = 0U; i < CONFIG_INT_BENCH_LOAD_NUM_THREADS; i++) {
		k_thread_create(&load_threads[i], load_stacks[i], LOAD_STACK_SIZE,
				load_thread_entry, (void *)load_buffers[i], NULL, NULL,
				priority + 1, 0, K_NO_WAIT);
	}
#endif
}

void bench_load_stop(void)
{
#ifdef CONFIG_INT_BENCH_LOAD_TIMER
	k_timer_stop(&load_timer);
#endif

#ifdef CONFIG_INT_BENCH_LOAD_THREADS
	load_running = false;

	for (uint32_t i = 0U; i < CONFIG_INT_BENCH_LOAD_NUM_THREADS; i++) {
		(void)k_thread_join(&load_threads[i], K_FOREVER);
	}
#endif
}

void bench_load_pollute(void)
{
#ifdef CONFIG_INT_BENCH_LOAD_CACHE
	for (size_t i = 0U; i < WORKING_SET_SIZE; i += WORKING_SET_STRIDE) {
		working_set[i]++;
	}
#endif
}

const char *bench_load_description(void)
{
	return ""
#ifdef CONFIG_INT_BENCH_LOAD_CACHE
		"cache "
#endif
#ifdef CONFIG_INT_BENCH_LOAD_TIMER
		"timer "
#endif
#ifdef CONFIG_INT_BENCH_LOAD_THREADS
		"threads "
#endif
		;
}

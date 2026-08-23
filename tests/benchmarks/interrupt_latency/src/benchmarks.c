/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Interrupt handling benchmarks, built on the ztest benchmark
 * framework. All scenarios are manually sampled benchmarks
 * (ZTEST_BENCHMARK_MANUAL) because their measured spans have endpoints
 * captured in different execution contexts (thread vs ISR), which
 * framework-timed benchmarks cannot express.
 *
 * The suite and all benchmarks live in one translation unit because
 * ZTEST_BENCHMARK_SUITE() defines the suite object static; individual
 * scenarios are selected with the CONFIG_INT_BENCH_SCENARIO_* options.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/irq.h>
#include <zephyr/timing/timing.h>

#include "trigger.h"

#define NUM_ITERATIONS CONFIG_INT_BENCH_NUM_ITERATIONS

static void suite_setup(void)
{
#ifdef CONFIG_INT_BENCH_TRIGGER_SW_IRQ
	printk("Interrupt benchmark: trigger=sw-irq (line %u), %u iterations\n",
	       bench_trigger_irq_line(), NUM_ITERATIONS);
#else
	printk("Interrupt benchmark: trigger=irq_offload, %u iterations\n"
	       "No sw-irq trigger for this architecture: only the exit path\n"
	       "scenarios run (entry latency, critical section, throughput\n"
	       "and dynamic connect need a real asynchronous interrupt)\n",
	       NUM_ITERATIONS);
#endif

	(void)bench_trigger_init();
}

ZTEST_BENCHMARK_SUITE(interrupt, suite_setup, NULL);

#if defined(CONFIG_INT_BENCH_SCENARIO_ENTRY) || defined(CONFIG_INT_BENCH_SCENARIO_EXIT) || \
	defined(CONFIG_INT_BENCH_SCENARIO_LOCKED)
static volatile bool fired;
#endif

#if defined(CONFIG_INT_BENCH_SCENARIO_ENTRY) || defined(CONFIG_INT_BENCH_SCENARIO_LOCKED)
/* Timestamp as early as possible in the ISR: measures the entry path */
static void entry_handler(void)
{
	ztest_benchmark_end();
	fired = true;
}
#endif

#ifdef CONFIG_INT_BENCH_SCENARIO_ENTRY
/*
 * Interrupt entry latency: time from the software write that raises
 * the interrupt to the first timestamp taken inside the ISR. Requires
 * the sw-irq trigger backend (a real asynchronous interrupt).
 */
static void entry_setup(void)
{
	bench_trigger_set_handler(entry_handler);
}

static void entry_teardown(void)
{
	bench_trigger_set_handler(NULL);
}

ZTEST_BENCHMARK_MANUAL(interrupt, entry_trigger_to_isr, NUM_ITERATIONS, entry_setup,
		       entry_teardown)
{
	fired = false;

	ztest_benchmark_start();
	bench_trigger();

	while (!fired) {
	}
}
#endif /* CONFIG_INT_BENCH_SCENARIO_ENTRY */

#ifdef CONFIG_INT_BENCH_SCENARIO_EXIT
/* Timestamp as the last operation in the ISR: measures the exit path */
static void exit_handler(void)
{
	fired = true;
	ztest_benchmark_start();
}

/*
 * Interrupt exit latency: time from the last instruction of the ISR
 * body back to the interrupted thread. Works with both trigger
 * backends; with irq_offload() it measures the offload trap exit path.
 */
static void exit_setup(void)
{
	bench_trigger_set_handler(exit_handler);
}

static void exit_teardown(void)
{
	bench_trigger_set_handler(NULL);
}

ZTEST_BENCHMARK_MANUAL(interrupt, exit_resume_interrupted, NUM_ITERATIONS, exit_setup,
		       exit_teardown)
{
	fired = false;

	bench_trigger();

	while (!fired) {
	}

	ztest_benchmark_end();
}

#define WAITER_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_SEM_DEFINE(wake_sem, 0, 1);
static K_SEM_DEFINE(sync_sem, 0, 1);
static K_THREAD_STACK_DEFINE(waiter_stack, WAITER_STACK_SIZE);
static struct k_thread waiter_thread;

static void resched_handler(void)
{
	k_sem_give(&wake_sem);
	ztest_benchmark_start();
}


static void waiter_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_take(&wake_sem, K_FOREVER);
	ztest_benchmark_end();
	k_sem_give(&sync_sem);
}

/*
 * Interrupt exit with rescheduling: time from the last instruction of
 * an ISR that wakes a higher priority thread to that thread running
 * (interrupt exit plus context switch).
 */
static void resched_setup(void)
{
	int priority = k_thread_priority_get(k_current_get());

	bench_trigger_set_handler(resched_handler);

	k_thread_create(&waiter_thread, waiter_stack, K_THREAD_STACK_SIZEOF(waiter_stack),
			waiter_entry, NULL, NULL, NULL, priority - 1, 0, K_NO_WAIT);
}

static void resched_teardown(void)
{
	(void)k_thread_join(&waiter_thread, K_FOREVER);
	bench_trigger_set_handler(NULL);
}

ZTEST_BENCHMARK_MANUAL(interrupt, exit_reschedule, NUM_ITERATIONS, resched_setup,
		       resched_teardown)
{
	bench_trigger();
	k_sem_take(&sync_sem, K_FOREVER);


}
#endif /* CONFIG_INT_BENCH_SCENARIO_EXIT */

#ifdef CONFIG_INT_BENCH_SCENARIO_LOCKED
/*
 * Entry latency after a critical section: the interrupt is raised
 * while interrupts are locked, kept pending for
 * CONFIG_INT_BENCH_LOCK_HOLD_US, then the time from irq_unlock() to
 * ISR entry is measured. Requires the sw-irq trigger backend.
 */
static void locked_setup(void)
{
	bench_trigger_set_handler(entry_handler);
}

static void locked_teardown(void)
{
	bench_trigger_set_handler(NULL);
}

ZTEST_BENCHMARK_MANUAL(interrupt, locked_unlock_to_isr, NUM_ITERATIONS, locked_setup,
		       locked_teardown)
{
	unsigned int key;

	fired = false;

	key = irq_lock();

	bench_trigger();
	k_busy_wait(CONFIG_INT_BENCH_LOCK_HOLD_US);

	ztest_benchmark_start();
	irq_unlock(key);

	while (!fired) {
	}
}
#endif /* CONFIG_INT_BENCH_SCENARIO_LOCKED */

#ifdef CONFIG_INT_BENCH_SCENARIO_THROUGHPUT
static volatile bool done;
static volatile uint32_t isr_count;

/*
 * Re-trigger from the top of the handler, so the next interrupt is
 * already pending while this one is still being serviced, and
 * timestamp the entry of both.
 */
static void throughput_handler(void)
{
	if (isr_count == 0U) {
		ztest_benchmark_start();
		isr_count = 1U;
		bench_trigger();
	} else {
		ztest_benchmark_end();
		done = true;
	}
}

static void throughput_setup(void)
{
	bench_trigger_set_handler(throughput_handler);
}

static void throughput_teardown(void)
{
	bench_trigger_set_handler(NULL);
}

/*
 * Sustained interrupt round-trip cost. The span runs from the entry of
 * one interrupt to the entry of the next, with the next already pending
 * throughout, so it covers the handler body, the exit path and the
 * entry path back to back (tail-chained where the hardware supports
 * it). Its inverse is the maximum sustainable interrupt rate. Requires
 * the sw-irq backend.
 */
ZTEST_BENCHMARK_MANUAL(interrupt, throughput_round_trip, NUM_ITERATIONS, throughput_setup,
		       throughput_teardown)
{
	isr_count = 0U;
	done = false;

	bench_trigger();

	while (!done) {
	}


}
#endif /* CONFIG_INT_BENCH_SCENARIO_THROUGHPUT */

#ifdef CONFIG_INT_BENCH_SCENARIO_DYNAMIC
/*
 * Cost of installing an interrupt handler at runtime with
 * irq_connect_dynamic(). The line is disabled while the handler is
 * (re)installed because z_isr_install() requires the line to be
 * disabled on most architectures.
 */
static void dynamic_setup(void)
{
	irq_disable(bench_trigger_irq_line());
}

static void dynamic_teardown(void)
{
	irq_enable(bench_trigger_irq_line());
}

ZTEST_BENCHMARK_MANUAL(interrupt, dynamic_connect, NUM_ITERATIONS, dynamic_setup,
		       dynamic_teardown)
{
	unsigned int line = bench_trigger_irq_line();

	ztest_benchmark_start();
	(void)irq_connect_dynamic(line, CONFIG_INT_BENCH_IRQ_PRIO, bench_trigger_isr, NULL, 0);
	ztest_benchmark_end();
}
#endif /* CONFIG_INT_BENCH_SCENARIO_DYNAMIC */

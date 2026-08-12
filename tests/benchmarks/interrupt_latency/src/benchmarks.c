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

#include "load.h"
#include "trigger.h"

#define NUM_ITERATIONS CONFIG_INT_BENCH_NUM_ITERATIONS

#if defined(CONFIG_INT_BENCH_SCENARIO_DIRECT) || defined(CONFIG_INT_BENCH_SCENARIO_ZLI)
static volatile bool alt_fired;

/*
 * Dispatched straight from the vector table: no software ISR table
 * lookup and none of the common entry code that wraps a regular ISR.
 * Returning zero tells the architecture layer that no reschedule is
 * needed, which keeps the exit path out of the measurement.
 */
ISR_DIRECT_DECLARE(bench_alt_isr)
{
	ztest_benchmark_end();
	alt_fired = true;

	return 0;
}
#endif

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

#ifdef CONFIG_INT_BENCH_SCENARIO_DIRECT
	IRQ_DIRECT_CONNECT(BENCH_IRQ_LINE_ALT, CONFIG_INT_BENCH_IRQ_PRIO, bench_alt_isr, 0);
	irq_enable(BENCH_IRQ_LINE_ALT);
#endif
#ifdef CONFIG_INT_BENCH_SCENARIO_ZLI
	/*
	 * Zero-latency interrupts have to be registered with
	 * IRQ_DIRECT_CONNECT(), and at a priority within the levels
	 * reserved for them.
	 */
	IRQ_DIRECT_CONNECT(BENCH_IRQ_LINE_ALT, 0, bench_alt_isr, IRQ_ZERO_LATENCY);
	irq_enable(BENCH_IRQ_LINE_ALT);
#endif

	printk("Background load: %s\n", bench_load_description());
	bench_load_start();
}

static void suite_teardown(void)
{
	bench_load_stop();
}

ZTEST_BENCHMARK_SUITE(interrupt, suite_setup, suite_teardown);

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
	bench_load_pollute();

	ztest_benchmark_start();
	bench_trigger();

	while (!fired) {
	}
}
#endif /* CONFIG_INT_BENCH_SCENARIO_ENTRY */

#if defined(CONFIG_INT_BENCH_SCENARIO_EXIT) || defined(CONFIG_INT_BENCH_SCENARIO_END_TO_END)
#define WAITER_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_SEM_DEFINE(wake_sem, 0, 1);
static K_SEM_DEFINE(sync_sem, 0, 1);
static K_THREAD_STACK_DEFINE(waiter_stack, WAITER_STACK_SIZE);
static struct k_thread waiter_thread;
#endif

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
	bench_load_pollute();

	bench_trigger();

	while (!fired) {
	}

	ztest_benchmark_end();
}

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
	bench_load_pollute();

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

	/*
	 * Pollute from inside the critical section, so that the
	 * interrupt is unmasked with the caches in the state a critical
	 * section doing real work would leave them in. This lengthens
	 * the hold time beyond the configured value.
	 */
	bench_load_pollute();

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
	bench_load_pollute();

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

	bench_load_pollute();

	ztest_benchmark_start();
	(void)irq_connect_dynamic(line, CONFIG_INT_BENCH_IRQ_PRIO, bench_trigger_isr, NULL, 0);
	ztest_benchmark_end();
}
#endif /* CONFIG_INT_BENCH_SCENARIO_DYNAMIC */

#ifdef CONFIG_INT_BENCH_SCENARIO_MASKING
/*
 * How long the system keeps interrupts masked is a property of the
 * kernel and its drivers rather than something the benchmark can
 * trigger, and Zephyr does not instrument irq_lock() windows, so
 * measure it by its effect: arm a one-shot timer for one tick, keep
 * the kernel busy taking spinlocks until it fires, and measure from
 * arming it to the first instruction of its ISR.
 *
 * The span is one nominal tick plus whatever delay the ISR suffered,
 * so the distribution reads directly: the median is the undelayed
 * period, and everything above it is delay that masking inflicted.
 * Reporting the interval rather than a delay computed against an
 * assumed period avoids having to know the counter frequency, which
 * timing_freq_get() does not report correctly on every platform.
 *
 * The number is the worst delay a periodic real-time event was
 * actually made to suffer, not the longest irq_lock() in the tree: a
 * window that does not overlap a tick boundary is never seen, and the
 * delay also contains time spent in interrupts served first.
 * CONFIG_INT_BENCH_MASK_INJECT_US masks for a known time so the
 * measurement can be checked on a new platform.
 */
static K_SEM_DEFINE(mask_sem, 0, 1);
static K_MUTEX_DEFINE(mask_mutex);

static volatile uint32_t mask_seq;

/*
 * The span is the interval between two expiries, so the first one opens
 * it and the second closes it. Both hooks are safe here: they only take
 * a timestamp, and the framework turns the span into a sample once the
 * body has returned to thread context.
 */
static void mask_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (mask_seq == 0U) {
		ztest_benchmark_start();
	} else if (mask_seq == 1U) {
		ztest_benchmark_end();
	}

	mask_seq++;
}

static K_TIMER_DEFINE(mask_timer, mask_timer_handler, NULL);

/* Kernel primitives that take spinlocks, and so mask interrupts */
static void mask_kernel_work(void)
{
	k_sem_give(&mask_sem);
	(void)k_sem_take(&mask_sem, K_NO_WAIT);
	(void)k_mutex_lock(&mask_mutex, K_NO_WAIT);
	(void)k_mutex_unlock(&mask_mutex);
	(void)k_uptime_get();
}

static void mask_setup(void)
{
	mask_seq = 0U;
	k_timer_start(&mask_timer, K_TICKS(1), K_TICKS(1));
}

static void mask_teardown(void)
{
	k_timer_stop(&mask_timer);
}

ZTEST_BENCHMARK_MANUAL(interrupt, periodic_isr_interval, NUM_ITERATIONS, mask_setup,
		       mask_teardown)
{
	int64_t deadline;

	/*
	 * Two expiries, so the span runs between consecutive periods of
	 * a running periodic timer rather than from an arbitrary arming
	 * point to the next tick boundary, which would carry up to a
	 * whole tick of phase noise. Each sample therefore costs two
	 * ticks, which is why this scenario needs the tick rate the load
	 * overlay configures rather than the much slower one the base
	 * configuration runs at.
	 *
	 * Give up rather than spin forever where the timer does not
	 * deliver at the tick rate at all, which under emulation can
	 * otherwise cost minutes of wall clock. Recording no sample
	 * leaves the benchmark inconclusive, which is the honest
	 * outcome.
	 */
	deadline = k_uptime_get() + 100;

	while (mask_seq < 2U) {
		if (k_uptime_get() > deadline) {
			return;
		}

		mask_kernel_work();

		if (CONFIG_INT_BENCH_MASK_INJECT_US > 0) {
			unsigned int key = irq_lock();

			k_busy_wait(CONFIG_INT_BENCH_MASK_INJECT_US);
			irq_unlock(key);
		}
	}
}
#endif /* CONFIG_INT_BENCH_SCENARIO_MASKING */

#if defined(CONFIG_INT_BENCH_SCENARIO_DIRECT) || defined(CONFIG_INT_BENCH_SCENARIO_ZLI)
/*
 * Bound on the spin waiting for the second line's ISR. A direct or
 * zero-latency interrupt that never arrives would otherwise hang the
 * run, and in the zero-latency case the wait happens with interrupts
 * locked, so nothing else could break the deadlock.
 */
#define ALT_SPIN_LIMIT 10000000U

static bool alt_wait(void)
{
	for (uint32_t spin = 0U; spin < ALT_SPIN_LIMIT; spin++) {
		if (alt_fired) {
			return true;
		}
	}

	return false;
}
#endif

/*
 * Entry latency of the second line, measured exactly as
 * entry_trigger_to_isr measures the first. What differs is only how
 * the line is connected, so the two are directly comparable.
 */
static void alt_entry_measure(const char *name)
{
	alt_fired = false;
	bench_load_pollute();

	ztest_benchmark_start();
	bench_trigger_alt();

	if (!alt_wait()) {
		printk("%s: ISR did not run, skipping\n", name);
		return;
	}
}

#ifdef CONFIG_INT_BENCH_SCENARIO_DIRECT
/*
 * A directly connected ISR is dispatched from the vector table, so the
 * difference from entry_trigger_to_isr is the software ISR table
 * dispatch and the common entry code a regular ISR goes through.
 */
ZTEST_BENCHMARK_MANUAL(interrupt, entry_direct_isr, NUM_ITERATIONS, NULL, NULL)
{
	alt_entry_measure("entry_direct_isr");
}
#endif /* CONFIG_INT_BENCH_SCENARIO_DIRECT */

#ifdef CONFIG_INT_BENCH_SCENARIO_ZLI
/*
 * Entry latency of a zero-latency interrupt raised while interrupts
 * are locked. A zero-latency interrupt runs above the priority the
 * kernel masks with, so unlike the interrupt in locked_unlock_to_isr
 * it is served inside the critical section rather than after it. The
 * two scenarios together show what a critical section costs an
 * interrupt, and what escaping it buys.
 */
/*
 * Entry latency of a zero-latency interrupt with interrupts enabled.
 * This is the lowest latency Zephyr offers, and being measured the
 * same way as entry_trigger_to_isr and entry_direct_isr it can be
 * compared with them; zli_entry_while_locked below answers the
 * different question of what happens inside a critical section.
 */
ZTEST_BENCHMARK_MANUAL(interrupt, zli_entry_trigger_to_isr, NUM_ITERATIONS, NULL, NULL)
{
	alt_entry_measure("zli_entry_trigger_to_isr");
}

ZTEST_BENCHMARK_MANUAL(interrupt, zli_entry_while_locked, NUM_ITERATIONS, NULL, NULL)
{
	unsigned int key;
	bool served;

	alt_fired = false;
	bench_load_pollute();

	key = irq_lock();

	ztest_benchmark_start();
	bench_trigger_alt();

	served = alt_wait();

	irq_unlock(key);

	if (!served) {
		printk("zli_entry_while_locked: not served while locked, skipping\n");
		return;
	}
}
#endif /* CONFIG_INT_BENCH_SCENARIO_ZLI */

#ifdef CONFIG_INT_BENCH_SCENARIO_END_TO_END
static void e2e_handler(void)
{
	k_sem_give(&wake_sem);
}

static void e2e_waiter_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_take(&wake_sem, K_FOREVER);
	ztest_benchmark_end();
	k_sem_give(&sync_sem);
}

/*
 * The whole path an application actually waits on: from raising the
 * interrupt to the high priority thread it wakes being on the CPU.
 *
 * The other scenarios measure the pieces of this span, but the pieces
 * cannot simply be added: entry latency and the rescheduling exit are
 * measured in separate runs and neither includes the ISR body or the
 * handoff between them. This is the figure to quote for how quickly an
 * application can respond to an event.
 */
static void e2e_setup(void)
{
	int priority = k_thread_priority_get(k_current_get());

	bench_trigger_set_handler(e2e_handler);

	k_thread_create(&waiter_thread, waiter_stack, K_THREAD_STACK_SIZEOF(waiter_stack),
			e2e_waiter_entry, NULL, NULL, NULL, priority - 1, 0, K_NO_WAIT);
}

static void e2e_teardown(void)
{
	(void)k_thread_join(&waiter_thread, K_FOREVER);
	bench_trigger_set_handler(NULL);
}

ZTEST_BENCHMARK_MANUAL(interrupt, irq_to_thread, NUM_ITERATIONS, e2e_setup, e2e_teardown)
{
	bench_load_pollute();

	ztest_benchmark_start();
	bench_trigger();

	k_sem_take(&sync_sem, K_FOREVER);
}
#endif /* CONFIG_INT_BENCH_SCENARIO_END_TO_END */

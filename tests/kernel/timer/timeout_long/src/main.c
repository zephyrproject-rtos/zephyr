/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq_offload.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#define TEST_CYCLES_PER_TICK 4U
#define TEST_LONG_TICKS UINT64_C(0x100000020)
#define TEST_SHORT_TICKS 7U
#define TEST_MAINTENANCE_IRQS 3U
#define TEST_IRQ_LATENCY_CYCLES 5U
#define TEST_MAX_PUMP_IRQS 128U

BUILD_ASSERT(IS_ENABLED(CONFIG_TEST_TIMEOUT_LONG_CLOCK));
BUILD_ASSERT(IS_ENABLED(CONFIG_TIMEOUT_64BIT));
BUILD_ASSERT(IS_ENABLED(CONFIG_TIMEOUT_BACKEND_DLIST));
BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC ==
	     TEST_CYCLES_PER_TICK * CONFIG_SYS_CLOCK_TICKS_PER_SEC);
BUILD_ASSERT(TEST_LONG_TICKS > UINT32_MAX);

static uint64_t reference_cycles;
static uint64_t armed_cycle;
static uint32_t delivered_irqs;

#if defined(CONFIG_TEST_TIMEOUT_LONG_64BIT_COMPARE)
typedef uint64_t test_counter_t;
#define TIMER_CORE_COUNTER_WIDTH 64
#define TIMER_CORE_ALARM_MAX_CYCLES UINT64_MAX
#define TEST_BUDGET_TRACE_CAPACITY 16U

static struct {
	uint64_t at;
	uint64_t compare;
} budget_arms[TEST_BUDGET_TRACE_CAPACITY];
static uint64_t budget_irqs[TEST_BUDGET_TRACE_CAPACITY];
static uint32_t budget_arm_count;
static uint32_t budget_irq_count;
static uint64_t last_announce_cycle;
static uint64_t maximum_announce_gap;
static bool observing_budget;
static bool budget_trace_overflow;
#else
typedef uint32_t test_counter_t;
#define TIMER_CORE_COUNTER_WIDTH 32
#endif

static test_counter_t timer_driver_cycle_get(void)
{
	return (test_counter_t)reference_cycles;
}

#if defined(CONFIG_TEST_TIMEOUT_LONG_RELOAD)
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_ALARM_MIN_CYCLES 8U

static void timer_driver_set_reload(uint32_t cycles)
{
	armed_cycle = reference_cycles + cycles;
}
#else
#define TIMER_CORE_BACKEND_COMPARE_EXACT

static void timer_driver_set_compare(test_counter_t cycles)
{
	test_counter_t ahead = cycles - (test_counter_t)reference_cycles;

	armed_cycle = reference_cycles + ahead;
#if defined(CONFIG_TEST_TIMEOUT_LONG_64BIT_COMPARE)
	if (observing_budget) {
		if (budget_arm_count < ARRAY_SIZE(budget_arms)) {
			budget_arms[budget_arm_count].at = reference_cycles;
			budget_arms[budget_arm_count].compare = armed_cycle;
			budget_arm_count++;
		} else {
			budget_trace_overflow = true;
		}
	}
#endif
}
#endif

#include "system_timer_generic.h"

static int test_clock_init(void)
{
	timer_core_init();
	return 0;
}

SYS_INIT(test_clock_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

static void test_clock_irq(const void *arg)
{
	ARG_UNUSED(arg);
	delivered_irqs++;
#if defined(CONFIG_TEST_TIMEOUT_LONG_64BIT_COMPARE)
	if (observing_budget) {
		maximum_announce_gap = MAX(maximum_announce_gap,
					  reference_cycles - last_announce_cycle);
		last_announce_cycle = reference_cycles;
		if (budget_irq_count < ARRAY_SIZE(budget_irqs)) {
			budget_irqs[budget_irq_count++] = reference_cycles;
		} else {
			budget_trace_overflow = true;
		}
	}
#endif
	timer_core_announce();
}

static void assert_clock_matches_reference(void)
{
	uint64_t expected_ticks = reference_cycles / TEST_CYCLES_PER_TICK;
	int64_t uptime = k_uptime_ticks();
	uint64_t cycles = k_cycle_get_64();

	zassert_equal(uptime, expected_ticks,
		      "uptime differs from input timeline: got %" PRId64 ", expected %" PRIu64,
		      uptime, expected_ticks);
	zassert_equal(cycles, reference_cycles,
		      "extended cycle count lost time: got %" PRIu64 ", expected %" PRIu64,
		      cycles, reference_cycles);
	zassert_equal(k_cycle_get_32(), (uint32_t)reference_cycles);
}

static void set_reference_cycles(uint64_t cycles)
{
	k_spinlock_key_t key = sys_clock_lock();

	reference_cycles = cycles;
	sys_clock_unlock(key);
}

static void deliver_next_irq(uint32_t late_cycles)
{
	zassert_true(armed_cycle > reference_cycles, "alarm is not in the future");
	set_reference_cycles(armed_cycle + late_cycles);
	irq_offload(test_clock_irq, NULL);
	assert_clock_matches_reference();
}

static void advance_to(uint64_t cycles)
{
	uint32_t irqs = 0;

	zassert_true(cycles >= reference_cycles, "input time moved backwards");
	while (reference_cycles < cycles) {
		zassert_true(armed_cycle > reference_cycles, "alarm is not in the future");
		if (armed_cycle > cycles) {
			set_reference_cycles(cycles);
			break;
		}
		zassert_true(irqs++ < TEST_MAX_PUMP_IRQS, "advance exceeded alarm bound");
		deliver_next_irq(0);
	}
	assert_clock_matches_reference();
}

struct timer_observation {
	uint32_t count;
	uint32_t order;
	int64_t tick;
	uint64_t cycle;
	bool in_isr;
};

static struct timer_observation long_observation;
static struct timer_observation short_observation;
static uint32_t callback_sequence;

static void record_expiry(struct k_timer *timer)
{
	struct timer_observation *observation = k_timer_user_data_get(timer);

	observation->count++;
	observation->order = ++callback_sequence;
	observation->tick = k_uptime_ticks();
	observation->cycle = reference_cycles;
	observation->in_isr = k_is_in_isr();
}

static K_TIMER_DEFINE(long_timer, record_expiry, NULL);
static K_TIMER_DEFINE(short_timer, record_expiry, NULL);

#if !defined(CONFIG_TEST_TIMEOUT_LONG_64BIT_COMPARE)
static void maintain_long_timeout(uint64_t deadline_tick)
{
	uint32_t before = delivered_irqs;

	for (uint32_t i = 0; i < TEST_MAINTENANCE_IRQS; i++) {
		zassert_true(armed_cycle + TEST_IRQ_LATENCY_CYCLES <
			     deadline_tick * TEST_CYCLES_PER_TICK,
			     "maintenance would pass the application deadline");
		deliver_next_irq(TEST_IRQ_LATENCY_CYCLES);
		zassert_equal(long_observation.count, 0, "long timeout fired during maintenance");
	}
	zassert_equal(delivered_irqs - before, TEST_MAINTENANCE_IRQS);
}
#endif

static void expect_expiry(struct k_timer *timer, const struct timer_observation *observation,
			  uint64_t deadline_tick)
{
	uint64_t deadline_cycle = deadline_tick * TEST_CYCLES_PER_TICK;

	advance_to(deadline_cycle - 1U);
	zassert_equal(observation->count, 0, "application timeout fired early");
	advance_to(deadline_cycle);
	zassert_equal(observation->count, 1, "application timeout was lost or duplicated");
	zassert_equal(observation->tick, deadline_tick, "callback tick differs from its deadline");
	zassert_equal(observation->cycle, deadline_cycle, "callback did not run at the alarm");
	zassert_true(observation->in_isr, "callback did not run in interrupt context");
	zassert_equal(k_timer_status_get(timer), 1);
	zassert_equal(k_timer_remaining_ticks(timer), 0);
}

static void *suite_setup(void)
{
	k_timer_user_data_set(&long_timer, &long_observation);
	k_timer_user_data_set(&short_timer, &short_observation);
	/* Consume the initial alarm, including RELOAD's two-tick minimum. */
	deliver_next_irq(0);
	return NULL;
}

static void before_test(void *fixture)
{
	ARG_UNUSED(fixture);
#if defined(CONFIG_TEST_TIMEOUT_LONG_64BIT_COMPARE)
	observing_budget = false;
	budget_arm_count = 0;
	budget_irq_count = 0;
	maximum_announce_gap = 0;
	budget_trace_overflow = false;
#endif
	k_timer_stop(&long_timer);
	k_timer_stop(&short_timer);
	(void)k_timer_status_get(&long_timer);
	(void)k_timer_status_get(&short_timer);
	long_observation = (struct timer_observation){0};
	short_observation = (struct timer_observation){0};
	callback_sequence = 0;
}

#if !defined(CONFIG_TEST_TIMEOUT_LONG_64BIT_COMPARE)
ZTEST(timeout_long, test_long_relative_timeout)
{
	uint64_t deadline = reference_cycles / TEST_CYCLES_PER_TICK + TEST_LONG_TICKS + 1U;

	k_timer_start(&long_timer, K_TICKS(TEST_LONG_TICKS), K_NO_WAIT);
	maintain_long_timeout(deadline);
	expect_expiry(&long_timer, &long_observation, deadline);
	zassert_equal(long_observation.order, 1);
	advance_to((deadline + 3U) * TEST_CYCLES_PER_TICK);
	zassert_equal(long_observation.count, 1);
	zassert_equal(k_timer_status_get(&long_timer), 0);
	zassert_equal(short_observation.count, 0);
}

ZTEST(timeout_long, test_short_during_long_timeout)
{
	uint64_t long_deadline = reference_cycles / TEST_CYCLES_PER_TICK + TEST_LONG_TICKS;

	k_timer_start(&long_timer, K_TIMEOUT_ABS_TICKS(long_deadline), K_NO_WAIT);
	maintain_long_timeout(long_deadline);

	uint64_t short_deadline = reference_cycles / TEST_CYCLES_PER_TICK + TEST_SHORT_TICKS + 1U;

	k_timer_start(&short_timer, K_TICKS(TEST_SHORT_TICKS), K_NO_WAIT);
	expect_expiry(&short_timer, &short_observation, short_deadline);
	zassert_equal(long_observation.count, 0);
	zassert_equal(short_observation.order, 1);
	expect_expiry(&long_timer, &long_observation, long_deadline);
	zassert_equal(long_observation.order, 2);
	advance_to((long_deadline + 3U) * TEST_CYCLES_PER_TICK);
	zassert_equal(long_observation.count, 1);
	zassert_equal(short_observation.count, 1);
	zassert_equal(k_timer_status_get(&long_timer), 0);
	zassert_equal(k_timer_status_get(&short_timer), 0);
}

ZTEST(timeout_long, test_short_after_long_timeout)
{
	uint64_t long_deadline = reference_cycles / TEST_CYCLES_PER_TICK + TEST_LONG_TICKS;

	k_timer_start(&long_timer, K_TIMEOUT_ABS_TICKS(long_deadline), K_NO_WAIT);
	maintain_long_timeout(long_deadline);
	expect_expiry(&long_timer, &long_observation, long_deadline);
	advance_to(reference_cycles + TEST_CYCLES_PER_TICK - 1U);

	uint64_t short_deadline = reference_cycles / TEST_CYCLES_PER_TICK + TEST_SHORT_TICKS + 1U;

	k_timer_start(&short_timer, K_TICKS(TEST_SHORT_TICKS), K_NO_WAIT);
	expect_expiry(&short_timer, &short_observation, short_deadline);
	zassert_equal(long_observation.order, 1);
	zassert_equal(short_observation.order, 2);
	advance_to((short_deadline + 3U) * TEST_CYCLES_PER_TICK);
	zassert_equal(long_observation.count, 1);
	zassert_equal(short_observation.count, 1);
	zassert_equal(k_timer_status_get(&long_timer), 0);
	zassert_equal(k_timer_status_get(&short_timer), 0);
}

#else
ZTEST(timeout_long, test_rearm_preserves_announce_budget)
{
	const uint64_t budget = SYS_CLOCK_MAX_WAIT;
	const uint64_t elapsed_ticks = budget - 32U;
	uint64_t prefix_deadline = reference_cycles / TEST_CYCLES_PER_TICK + 2U;

	k_timer_start(&short_timer, K_TICKS(1), K_NO_WAIT);
	expect_expiry(&short_timer, &short_observation, prefix_deadline);
	short_observation = (struct timer_observation){0};
	callback_sequence = 0;

	uint64_t start_cycle = reference_cycles;
	uint64_t start_tick = start_cycle / TEST_CYCLES_PER_TICK;
	uint64_t elapsed_cycle = start_cycle + elapsed_ticks * TEST_CYCLES_PER_TICK;
	uint64_t budget_cycle = start_cycle + budget * TEST_CYCLES_PER_TICK;
	uint32_t before = delivered_irqs;

	zassert_equal(start_cycle % TEST_CYCLES_PER_TICK, 0, "baseline is not tick aligned");
	last_announce_cycle = start_cycle;
	observing_budget = true;
	k_timer_start(&long_timer, K_TICKS(2U * budget + 64U), K_NO_WAIT);
	zassert_true(armed_cycle > elapsed_cycle, "an alarm is due before the chosen elapsed");
	advance_to(elapsed_cycle);
	zassert_equal(delivered_irqs, before, "an intermediate announce reset the budget");
	zassert_true(armed_cycle > reference_cycles, "an armed alarm was skipped");

	uint32_t first_rearm = budget_arm_count;

	k_timer_start(&long_timer, K_TICKS(budget + 64U), K_NO_WAIT);

	uint32_t after_rearm = budget_arm_count;
	uint64_t rearm_cycle = armed_cycle;
	uint64_t long_deadline = start_tick + elapsed_ticks + budget + 65U;

	expect_expiry(&long_timer, &long_observation, long_deadline);
	k_timer_start(&short_timer, K_TICKS(TEST_SHORT_TICKS), K_NO_WAIT);
	expect_expiry(&short_timer, &short_observation, long_deadline + TEST_SHORT_TICKS + 1U);
	observing_budget = false;

	TC_PRINT("BUDGET base=%" PRIu64 " elapsed=%" PRIu64 " rearm=%" PRIu64
		 " max_gap=%" PRIu64 " arms=%u irqs=%u\n",
		 start_cycle, elapsed_cycle, rearm_cycle, maximum_announce_gap,
		 budget_arm_count, budget_irq_count);
	for (uint32_t i = 0; i < budget_arm_count; i++) {
		TC_PRINT("ARM at=%" PRIu64 " compare=%" PRIu64 "\n",
			 budget_arms[i].at, budget_arms[i].compare);
	}
	for (uint32_t i = 0; i < budget_irq_count; i++) {
		TC_PRINT("IRQ at=%" PRIu64 "\n", budget_irqs[i]);
	}

	zassert_false(budget_trace_overflow, "clock trace overflowed");
	zassert_equal(long_observation.count, 1);
	zassert_equal(short_observation.count, 1);
	zassert_equal(long_observation.order, 1);
	zassert_equal(short_observation.order, 2);
	zassert_true(maximum_announce_gap <= budget * TEST_CYCLES_PER_TICK,
		     "announce gap exceeds kernel budget");
	zassert_true(after_rearm > first_rearm, "rearm did not program the comparator");
	zassert_true(rearm_cycle <= budget_cycle, "rearm moved the budget deadline");
	for (uint32_t i = first_rearm; i < after_rearm; i++) {
		zassert_equal(budget_arms[i].at, elapsed_cycle);
		zassert_true(budget_arms[i].compare <= budget_cycle,
			     "a rearm exceeded the remaining announce budget");
	}
}
#endif

ZTEST_SUITE(timeout_long, NULL, suite_setup, before_test, NULL, NULL);

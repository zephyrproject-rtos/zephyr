/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Unit tests for the generic system-timer core
 *
 * The core is compiled in the way a driver compiles it, against a simulated
 * counter, with the calls it makes out of the core mocked. That makes its
 * announce baseline visible to the tests.
 *
 * The main thing checked is that the two conversion directions agree. When
 * they do not, announced time still looks right, but some interrupts announce
 * nothing.
 *
 * The core defines the global sys_clock_* entry points, so one build covers
 * one configuration. tests.yaml lists the scenarios.
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/timer/system_timer.h>

/* The simulated hardware for this scenario, from Kconfig, as TC_* macros. */
#define TC_WIDTH CONFIG_TEST_COUNTER_WIDTH

#if CONFIG_TEST_ALARM_MAX_CYCLES != 0
#define TC_ALARM_MAX CONFIG_TEST_ALARM_MAX_CYCLES
#endif

#if CONFIG_TEST_ALARM_MIN_CYCLES != 0
#define TC_ALARM_MIN CONFIG_TEST_ALARM_MIN_CYCLES
#endif

#if defined(CONFIG_TEST_BACKEND_COMPARE_EXACT)
#define TC_BACKEND_COMPARE_EXACT
#elif defined(CONFIG_TEST_BACKEND_RELOAD)
#define TC_BACKEND_RELOAD
#endif

#if defined(CONFIG_TEST_COUNTER_NONATOMIC)
#define TC_NONATOMIC
#endif

#if defined(CONFIG_TEST_COUNTER_NONMONOTONIC)
#define TC_NONMONOTONIC
#endif

#if defined(CONFIG_TEST_RATE_FROM_DRIVER)
#define TC_RUNTIME_RATE
#endif

/* The bits of the count the hardware register holds. */
#define TC_MASK (UINT64_MAX >> (64 - TC_WIDTH))

#if TC_WIDTH <= 32
typedef uint32_t tc_hw_t;
#else
typedef uint64_t tc_hw_t;
#endif

/*
 * Simulated hardware.
 *
 * tc_now is the absolute cycle count, never truncated. The driver hooks return
 * it masked to TC_WIDTH, like a narrow counter would, and the core has to
 * restore the high bits. tc_fire is the absolute cycle of the next interrupt.
 */
#define TC_NEVER UINT64_MAX

static uint64_t tc_now;
static uint64_t tc_fire = TC_NEVER;
static uint64_t tc_reload_period;
static tc_hw_t tc_compare;
static unsigned long tc_arm_writes;

static inline tc_hw_t timer_driver_cycle_get(void)
{
	return (tc_hw_t)(tc_now & TC_MASK);
}

#if defined(TC_BACKEND_RELOAD)

#define TIMER_CORE_BACKEND_RELOAD

static inline void timer_driver_set_reload(tc_hw_t cycles)
{
#if defined(TC_ALARM_MAX)
	/* A value past the register's range would be truncated by real
	 * hardware. The core must clamp before calling.
	 */
	zassert_true(cycles <= TC_ALARM_MAX, "reload of %llu past the %llu the register holds",
		     (unsigned long long)cycles, (unsigned long long)TC_ALARM_MAX);
#endif
#if defined(TC_ALARM_MIN)
	/* The hardware cannot be armed nearer than its floor. The core must
	 * bump shorter requests up to it.
	 */
	zassert_true(cycles >= TC_ALARM_MIN, "reload of %llu under the %llu floor",
		     (unsigned long long)cycles, (unsigned long long)TC_ALARM_MIN);
#endif
	tc_arm_writes++;
	/* Programming a reload restarts the counter, and the hardware then
	 * reloads the same value on its own.
	 */
	tc_reload_period = cycles;
	tc_fire = tc_now + cycles;
}

#else

#if defined(TC_BACKEND_COMPARE_EXACT)
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#else
#define TIMER_CORE_BACKEND_COMPARE_ORDERED
#endif

static inline void timer_driver_set_compare(tc_hw_t cycles)
{
	uint64_t ahead;

	tc_arm_writes++;
	tc_compare = (tc_hw_t)(cycles & TC_MASK);
	ahead = (tc_compare - tc_now) & TC_MASK;

#if defined(TC_ALARM_MAX)
	/* Check the reach only for targets ahead of the counter. A target in
	 * the far half of the window has already been passed, which the
	 * backends are allowed to produce.
	 */
	if (ahead <= (TC_MASK >> 1)) {
		zassert_true(ahead <= TC_ALARM_MAX,
			     "compare set %llu cycles out, past the %llu the alarm reaches",
			     (unsigned long long)ahead, (unsigned long long)TC_ALARM_MAX);
	}
#endif

#if defined(TC_BACKEND_COMPARE_EXACT)
	/* Equality only: a value at or behind the counter is missed until the
	 * counter wraps around. The core's verify loop must prevent that, so it
	 * is modeled as is.
	 */
	tc_fire = tc_now + (ahead == 0U ? (TC_MASK + 1U) : ahead);
#else
	/* Ordered comparison: a value at or behind the counter fires at once.
	 * Behind means the far half of the masked window.
	 */
	tc_fire = (ahead > (TC_MASK >> 1)) ? tc_now : tc_now + ahead;
#endif
}

#endif /* TC_BACKEND_RELOAD */

#if defined(TC_BACKEND_RELOAD)
#define TC_BACKEND_NAME "RELOAD"
#elif defined(TC_BACKEND_COMPARE_EXACT)
#define TC_BACKEND_NAME "COMPARE_EXACT"
#else
#define TC_BACKEND_NAME "COMPARE_ORDERED"
#endif

#define TIMER_CORE_COUNTER_WIDTH TC_WIDTH

#if defined(TC_NONMONOTONIC)
#define TIMER_CORE_COUNTER_NONMONOTONIC
#endif

#if defined(TC_NONATOMIC)
#define TIMER_CORE_COUNTER_NONATOMIC
#endif

#if defined(TC_ALARM_MAX)
/* An alarm narrower than the counter, as when the counter is extended in
 * software and the alarm is the hardware's own.
 */
#define TIMER_CORE_ALARM_MAX_CYCLES TC_ALARM_MAX
#endif

#if defined(TC_ALARM_MIN)
/* The alarm cannot be armed nearer than this. */
#define TIMER_CORE_ALARM_MIN_CYCLES TC_ALARM_MIN
#endif

/*
 * A rate the build cannot see comes two ways: the kernel reads it at run time,
 * or the driver states a rate of its own held in a variable. Each gets a
 * scenario.
 */
#if defined(TC_RUNTIME_RATE)
/*
 * A driver stating its own rate typically runs its counter faster than the
 * kernel's cycle unit. The kernel then cannot read the counter directly, so the
 * core provides no getter and the driver supplies one that scales. A whole
 * multiple keeps that scaling exact, so only the tick accounting is under test.
 */
#define TC_DRIVER_MULT 4U
#else
#define TC_DRIVER_MULT 1U
#endif

/* Counter cycles in the kernel's cycle unit. */
#define TC_KERNEL_CYCLES(c) ((c) / TC_DRIVER_MULT)

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || defined(TC_RUNTIME_RATE)
#define TC_RATE_IS_VARIABLE 1
static uint32_t tc_rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC * TC_DRIVER_MULT;
#else
#define TC_RATE_IS_VARIABLE 0
#endif

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
unsigned int sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	return tc_rate;
}
#elif defined(TC_RUNTIME_RATE)
#define TIMER_CORE_CYCLES_PER_SEC tc_rate
#define TIMER_CORE_CYCLES_PER_SEC_RUNTIME
#endif

#if defined(TC_RUNTIME_RATE)
/* A driver stating its own rate must provide the getters, so the core emits
 * neither. They are defined after the include, where timer_core_cycle_get() is
 * available. Every other scenario tests the core's own getters.
 */
#define TIMER_CORE_HAVE_CYCLE_GET_32
#endif

/*
 * Mocked calls out of the core. On a uniprocessor build sys_clock_lock() and
 * sys_clock_unlock() are static inline wrappers around arch_irq_lock(), so only
 * the functions below need mocking.
 */
static uint64_t tc_announced;
static unsigned long tc_announce_calls;
static unsigned long tc_announce_empty;
static bool tc_kernel_rearms;
static uint32_t tc_next_timeout(void);

bool sys_clock_is_locked(void)
{
	return true;
}

/* The unit build has no LPM companion Kconfig, so the core always calls this. */
void z_sys_clock_lpm_init(void)
{
}

void sys_clock_announce_locked(uint32_t ticks, k_spinlock_key_t key)
{
	ARG_UNUSED(key);

	tc_announced += ticks;
	tc_announce_calls++;
	if (ticks == 0U) {
		tc_announce_empty++;
	}

	/* Like the kernel, compute the next deadline and reprogram, so the core
	 * sees calls in the same order as in a real system.
	 */
	if (tc_kernel_rearms) {
		sys_clock_set_timeout(tc_next_timeout(), false);
	}
}

#include "system_timer_generic.h"

#if defined(TC_RUNTIME_RATE)
/* The core's count, extended past the hardware wrap, scaled to the kernel's
 * cycle unit.
 */
uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)TC_KERNEL_CYCLES(timer_core_cycle_get());
}

uint64_t sys_clock_cycle_get_64(void)
{
	return TC_KERNEL_CYCLES(timer_core_cycle_get());
}
#endif

#define TC_TICK_RATE ((uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Arms needed to cross one tick. More than one only when the alarm cannot reach
 * a whole tick, and then every arm but the last announces nothing.
 */
#define TC_ARMS_PER_TICK                                                                           \
	((uint64_t)DIV_ROUND_UP(TIMER_CORE_CYC_PER_TICK, TIMER_CORE_MAX_ARM_CYCLES))

/* Hardware cycle rate, taken independently of the core. */
#if TC_RATE_IS_VARIABLE
#define TC_CYCLE_RATE ((uint64_t)tc_rate)
#else
#define TC_CYCLE_RATE ((uint64_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#endif

/*
 * Reference conversion, independent of the core's: whole ticks started by cycle
 * @p c, counted from cycle zero. Tick n starts at n * rate / ticks_per_sec, so
 * this is the floor of the inverse. After a rescale the counter is re-expressed
 * from tick zero, so no anchor is needed.
 *
 * Split at the hardware cycle rate so c * TC_TICK_RATE is never formed: the
 * long runs reach cycle counts where that product overflows 64 bits.
 */
static uint64_t tc_ref_ticks(uint64_t c)
{
	return ((c / TC_CYCLE_RATE) * TC_TICK_RATE) +
	       (((c % TC_CYCLE_RATE) * TC_TICK_RATE) / TC_CYCLE_RATE);
}

/* Deterministic timeout schedule, so a failure reproduces. */
static uint64_t tc_rand_state;

static uint32_t tc_rand(uint32_t mod)
{
	tc_rand_state = (tc_rand_state * 6364136223846793005ULL) + 1442695040888963407ULL;
	return (uint32_t)((tc_rand_state >> 33) % mod);
}

static uint32_t tc_timeout_span = 1;

static uint32_t tc_next_timeout(void)
{
	return 1U + tc_rand(tc_timeout_span);
}

static void tc_reset_at(uint64_t start, uint32_t timeout_span, bool rearm)
{
	tc_now = start;
	tc_fire = TC_NEVER;
	tc_reload_period = 0;
	tc_compare = 0;
	tc_announced = 0;
	tc_announce_calls = 0;
	tc_announce_empty = 0;
	tc_arm_writes = 0;
	tc_rand_state = 20260911;
	tc_timeout_span = timeout_span;
	tc_kernel_rearms = rearm;

	timer_core_last_cycle = 0;
	timer_core_last_tick = 0;
	timer_core_last_elapsed = 0;
#if !TIMER_CORE_TICK_IS_WHOLE
	timer_core_last_rem = 0;
#endif
#if defined(TIMER_CORE_BACKEND_RELOAD)
	/* The arm path skips the hardware when asked for the deadline already
	 * programmed, so a deadline left by the previous test would leave this
	 * one unarmed.
	 */
	timer_core_armed_deadline = UINT64_MAX;
	timer_core_catchup = false;
#endif
	timer_core_init();

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
#if defined(TIMER_CORE_BACKEND_RELOAD)
		/* A tickful RELOAD driver sets its own period and the core leaves
		 * the hardware alone, so free-run it at one tick as such a driver's
		 * init would.
		 */
		timer_driver_set_reload((tc_hw_t)TIMER_CORE_CYC_PER_TICK);
#endif
	}
}

static void tc_reset(uint32_t timeout_span, bool rearm)
{
	tc_reset_at(0, timeout_span, rearm);
}

/*
 * The baseline sits on a tick position. timer_core_cyc_at_tick() computes that
 * position directly, while the announce path gets there incrementally, so
 * agreement means the increments have not accumulated an error.
 */
static void tc_check_baseline(void)
{
	timer_core_ticks_t ceiling = timer_core_ticks_at_cyc(TIMER_CORE_MAX_UNANNOUNCED_CYCLES);

	zassert_equal(timer_core_last_cycle, timer_core_cyc_at_tick(timer_core_last_tick),
		      "baseline %llu is not the position of tick %llu (%llu)",
		      (unsigned long long)timer_core_last_cycle,
		      (unsigned long long)timer_core_last_tick,
		      (unsigned long long)timer_core_cyc_at_tick(timer_core_last_tick));

	/* The arm ceiling is the span the counter resolves, in ticks, and must
	 * still hold at the current rate. Where it is computed once, a rate change
	 * since would leave it too large.
	 */
	zassert_true(ceiling >= TIMER_CORE_MAX_SPAN_TICKS,
		     "the arm ceiling of %llu ticks reaches past the %llu the counter resolves",
		     (unsigned long long)TIMER_CORE_MAX_SPAN_TICKS, (unsigned long long)ceiling);
}

/*
 * Announced time matches the counter exactly. This holds between announces too:
 * the kernel reads the clock as the announced count plus sys_clock_elapsed().
 */
static void tc_check_clock(void)
{
	uint64_t want = tc_ref_ticks(tc_now);
	uint64_t seen = timer_core_last_tick + sys_clock_elapsed();

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		zassert_equal(seen, want, "clock reads %llu ticks at cycle %llu, want %llu",
			      (unsigned long long)seen, (unsigned long long)tc_now,
			      (unsigned long long)want);
		return;
	}

	/* A tickful kernel only moves the clock at the interrupt, so it may lag
	 * by up to the undelivered period but must never run ahead.
	 */
	zassert_true(seen <= want, "clock ran ahead of the counter");
	zassert_true((want - seen) <= 2U, "clock lags %llu ticks behind the counter",
		     (unsigned long long)(want - seen));
}

/* Run the timer until @p cycles have passed, announcing at every interrupt. */
static void tc_run(uint64_t cycles)
{
	uint64_t end = tc_now + cycles;

	while (tc_now < end) {
		zassert_not_equal(tc_fire, TC_NEVER, "timer left unarmed at cycle %llu",
				  (unsigned long long)tc_now);
		zassert_true(tc_fire >= tc_now, "interrupt scheduled in the past");

		tc_now = tc_fire;
#if defined(TC_BACKEND_RELOAD)
		tc_fire = tc_now + tc_reload_period;
#else
		tc_fire = TC_NEVER;
#endif
		timer_core_announce();

		tc_check_baseline();
		zassert_equal(timer_core_last_tick, tc_ref_ticks(tc_now),
			      "announced %llu ticks at cycle %llu, want %llu",
			      (unsigned long long)timer_core_last_tick, (unsigned long long)tc_now,
			      (unsigned long long)tc_ref_ticks(tc_now));
	}
}

ZTEST(timer_core, test_conversions_round_trip)
{
	/*
	 * The span to tick n reads back as exactly n ticks, and one cycle less as
	 * n - 1. A conversion pair that fails this still keeps time, but takes an
	 * extra interrupt per tick that announces nothing.
	 *
	 * Checked at every baseline phase, since last_rem cycles through its whole
	 * range.
	 */
	tc_reset(1, false);

	for (uint32_t step = 0; step < 2000U; step++) {
		tc_check_baseline();

		for (uint32_t k = 0; k <= 64U; k++) {
			timer_core_cycles_t span = timer_core_span_cycles(k);

			zassert_equal(timer_core_ticks_in(span), k,
				      "%u ticks span %llu cycles, which reads back as %llu", k,
				      (unsigned long long)span,
				      (unsigned long long)timer_core_ticks_in(span));

			if (span > 0U) {
				zassert_equal(timer_core_ticks_in(span - 1U), k > 0U ? k - 1U : 0U,
					      "a cycle short of tick %u reads as %llu", k,
					      (unsigned long long)timer_core_ticks_in(span - 1U));
			}
		}

		timer_core_advance_baseline(1U + (step % 7U));
	}
}

ZTEST(timer_core, test_tick_positions_are_monotonic)
{
	/* Successive ticks are at least one cycle apart and never go backwards,
	 * whatever the ratio. Two ticks on the same cycle could not be announced
	 * separately.
	 */
	uint64_t prev;

	tc_reset(1, false);
	prev = timer_core_cyc_at_tick(0);

	for (uint64_t n = 1; n < 100000U; n++) {
		uint64_t at = timer_core_cyc_at_tick(n);

		zassert_true(at > prev, "tick %llu is not after tick %llu", (unsigned long long)n,
			     (unsigned long long)(n - 1));
		zassert_equal(at, prev + timer_core_span_cycles(1),
			      "tick %llu is not one span past the baseline", (unsigned long long)n);
		prev = at;
		timer_core_advance_baseline(1);
	}
}

ZTEST(timer_core, test_every_tick_announced)
{
	/* One tick at a time, as a busy system asks for. Every interrupt must
	 * carry a tick: an empty announce is a wasted interrupt, and mismatched
	 * conversions produce a stream of them while still keeping time.
	 *
	 * The empty-announce check is tickless only. A tickful kernel never
	 * reprograms, so with a tick that is not a whole number of cycles no fixed
	 * period is right.
	 */
	tc_reset(1, IS_ENABLED(CONFIG_TICKLESS_KERNEL));
	tc_run(TC_CYCLE_RATE * 30U);

	zassert_true(tc_announced > 0, "no ticks announced");

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		zassert_true(tc_announce_empty <= ((tc_announced + 1U) * (TC_ARMS_PER_TICK - 1U)),
			     "%lu of %lu interrupts announced nothing, more than the %llu that "
			     "crossing %llu ticks an arm at a time can account for",
			     tc_announce_empty, tc_announce_calls,
			     (unsigned long long)((tc_announced + 1U) * (TC_ARMS_PER_TICK - 1U)),
			     (unsigned long long)tc_announced);
	}
}

ZTEST(timer_core, test_long_run_keeps_time)
{
	/* A day, crossing a 24-bit counter's wrap thousands of times. tc_run()
	 * checks every interrupt, so this catches slow accumulation, such as a
	 * baseline that drifts by a cycle per wrap.
	 */
	tc_reset(64, IS_ENABLED(CONFIG_TICKLESS_KERNEL));
	tc_run(TC_CYCLE_RATE * 86400U);

	zassert_equal(timer_core_last_tick, tc_ref_ticks(tc_now), "a day of ticks does not add up");
	zassert_equal(tc_announced, timer_core_last_tick,
		      "announced total does not match the baseline");
}

ZTEST(timer_core, test_clock_read_between_announces)
{
	/* Mid-tick reads: the announced count plus sys_clock_elapsed() matches
	 * the counter at any cycle, not only at interrupts.
	 */
	tc_reset(16, IS_ENABLED(CONFIG_TICKLESS_KERNEL));

	for (uint32_t i = 0; i < 20000U; i++) {
		uint64_t step = 1U + tc_rand(3U * (uint32_t)TIMER_CORE_CYC_PER_TICK);

		/* Stop at the armed deadline: past it the interrupt is pending and
		 * the clock is legitimately behind.
		 */
		if ((tc_fire != TC_NEVER) && ((tc_now + step) >= tc_fire)) {
			tc_now = tc_fire;
#if defined(TC_BACKEND_RELOAD)
			tc_fire = tc_now + tc_reload_period;
#else
			tc_fire = TC_NEVER;
#endif
			timer_core_announce();
			tc_check_baseline();
		} else {
			tc_now += step;
		}

		tc_check_clock();
	}
}

ZTEST(timer_core, test_late_interrupt)
{
	/* A starved ISR: the counter is well past the deadline when the announce
	 * runs. The whole span must be announced, not one tick.
	 */
	tc_reset(1, IS_ENABLED(CONFIG_TICKLESS_KERNEL));

	for (uint32_t i = 0; i < 200U; i++) {
		uint64_t late = TIMER_CORE_CYC_PER_TICK * (1U + tc_rand(20U));

		zassert_not_equal(tc_fire, TC_NEVER, "timer left unarmed");
		tc_now = tc_fire + late;
#if defined(TC_BACKEND_RELOAD)
		tc_fire = tc_now + tc_reload_period;
#else
		tc_fire = TC_NEVER;
#endif
		timer_core_announce();

		tc_check_baseline();
		zassert_equal(timer_core_last_tick, tc_ref_ticks(tc_now),
			      "a late announce lost time at cycle %llu",
			      (unsigned long long)tc_now);
	}
}

ZTEST(timer_core, test_far_deadline_walks)
{
	/* A deadline beyond one arm's reach is reached over several arms, each
	 * announcing what it covered. No arm may cross more than the counter
	 * resolves, and the clock must stay exact throughout.
	 *
	 * Tickless only: a tickful kernel sets no deadline.
	 */
	uint64_t start_tick;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ztest_test_skip();
	}

	tc_reset(1, false);
	start_tick = timer_core_last_tick;

	for (uint32_t arm = 0; arm < (12U * TC_ARMS_PER_TICK); arm++) {
		uint64_t before = timer_core_last_tick;

		sys_clock_set_timeout(UINT32_MAX, false);
		zassert_not_equal(tc_fire, TC_NEVER, "timer left unarmed mid-walk");

		tc_now = tc_fire;
#if defined(TC_BACKEND_RELOAD)
		tc_fire = tc_now + tc_reload_period;
#else
		tc_fire = TC_NEVER;
#endif
		timer_core_announce();
		tc_check_baseline();

		/* An arm that cannot reach the next tick announces nothing: it makes
		 * progress in cycles, not ticks.
		 */
		zassert_true(timer_core_last_tick >= before, "an arm lost time");
		zassert_true((timer_core_last_tick - before) <= (uint64_t)TIMER_CORE_MAX_SPAN_TICKS,
			     "an arm crossed %llu ticks, past the %llu the counter resolves",
			     (unsigned long long)(timer_core_last_tick - before),
			     (unsigned long long)TIMER_CORE_MAX_SPAN_TICKS);
		zassert_equal(timer_core_last_tick, tc_ref_ticks(tc_now),
			      "the walk lost time at cycle %llu", (unsigned long long)tc_now);
	}

	zassert_true(timer_core_last_tick > start_tick, "the walk reached no tick at all");
}

ZTEST(timer_core, test_recovery_announce)
{
	/* A span recovered by the driver, wider than the counter. Only whole
	 * ticks are announced, and the remainder carries over.
	 */
	uint64_t before;

	tc_reset(1, false);
	before = timer_core_last_tick;

	timer_core_announce_cycles64_from(sys_clock_lock(), TC_CYCLE_RATE * 600U);

	zassert_equal(timer_core_last_tick - before, (uint64_t)TC_TICK_RATE * 600U,
		      "recovered %llu ticks from ten minutes of cycles",
		      (unsigned long long)(timer_core_last_tick - before));
	tc_check_baseline();
}

ZTEST(timer_core, test_rescale_keeps_time)
{
	/* A driver changing its hardware cycle rate rescales its own count and
	 * tells the core, which re-expresses the baseline at the new rate. The
	 * tick count must not change, and everything derived from the rate must
	 * follow.
	 */
#if !TC_RATE_IS_VARIABLE
	/* A build-time rate cannot change. */
	ztest_test_skip();
#else
	uint32_t from_hz;
	uint32_t to_hz;
	uint64_t frac;
	uint64_t tick_at_change;

	tc_reset(8, IS_ENABLED(CONFIG_TICKLESS_KERNEL));
	tc_run(TC_CYCLE_RATE * 5U);

	tick_at_change = timer_core_last_tick;
	frac = tc_now - timer_core_last_cycle;
	from_hz = tc_rate;
	to_hz = from_hz * 3U;

	/* Raise the rate rather than lower it: values derived from the old rate
	 * would then overstate what the counter can span.
	 */
	tc_rate = to_hz;
	timer_core_rescale(to_hz, from_hz);

	/* The driver's side: rescale the counter around the baseline the core
	 * just placed. The part past the tick scales with the rate.
	 */
	tc_now = timer_core_last_cycle + ((frac * to_hz) / from_hz);

	zassert_equal(timer_core_last_tick, tick_at_change,
		      "the rescale moved the clock from %llu to %llu ticks",
		      (unsigned long long)tick_at_change, (unsigned long long)timer_core_last_tick);
	tc_check_baseline();
	zassert_equal(timer_core_last_tick, tc_ref_ticks(tc_now),
		      "the clock does not read the rescaled counter");

	/* Time keeps running at the new rate. */
	sys_clock_set_timeout(1, false);
	tc_run(TC_CYCLE_RATE * 5U);
#endif
}

ZTEST(timer_core, test_cycle_get_extends_past_wrap)
{
	/* The kernel's two cycle getters. The core extends a counter narrower
	 * than their result from the baseline, so check across the hardware
	 * wrap.
	 */
	uint64_t prev64;
	uint64_t start = 0;
	uint64_t end;

#if TC_WIDTH < 64
	/* Start 10 ms before the wrap, so the run crosses it early and goes
	 * well past it.
	 */
	start = ((uint64_t)TC_MASK + 1U) - (TC_CYCLE_RATE / 100U);
#endif
	tc_reset_at(start, 4, IS_ENABLED(CONFIG_TICKLESS_KERNEL));
	end = tc_now + TC_CYCLE_RATE;
	prev64 = sys_clock_cycle_get_64();

	while (tc_now < end) {
		uint64_t cyc64;
		uint32_t cyc32;
		uint64_t step = 1U + tc_rand(2U * (uint32_t)TIMER_CORE_CYC_PER_TICK);

		if ((tc_fire != TC_NEVER) && ((tc_now + step) >= tc_fire)) {
			tc_now = tc_fire;
#if defined(TC_BACKEND_RELOAD)
			tc_fire = tc_now + tc_reload_period;
#else
			tc_fire = TC_NEVER;
#endif
			timer_core_announce();
		} else {
			tc_now += step;
		}

		cyc64 = sys_clock_cycle_get_64();
		cyc32 = sys_clock_cycle_get_32();

		zassert_equal(cyc64, TC_KERNEL_CYCLES(tc_now),
			      "the 64-bit getter reads %llu at cycle %llu",
			      (unsigned long long)cyc64, (unsigned long long)tc_now);
		zassert_equal(cyc32, (uint32_t)TC_KERNEL_CYCLES(tc_now),
			      "the 32-bit getter reads %u at cycle %llu", cyc32,
			      (unsigned long long)tc_now);
		zassert_true(cyc64 >= prev64, "the 64-bit getter went backwards");
		prev64 = cyc64;
	}

#if TC_WIDTH < 64
	zassert_true(tc_now > (uint64_t)TC_MASK, "the run did not reach the wrap");
#endif
}

ZTEST(timer_core, test_reload_floor_holds_off_a_storm)
{
	/* With a floor, the core cannot arm a deadline already due, so it arms
	 * the floor instead. Programming a reload restarts the counter, so
	 * set_timeout() calls arriving faster than the floor, as from a
	 * syscall-heavy thread resetting its timeslice, would keep pushing the
	 * interrupt out and freeze time. The core must write the hardware once
	 * and leave the pending interrupt alone until it is announced.
	 */
#if !defined(TC_BACKEND_RELOAD)
	ztest_test_skip();
#else
	unsigned long writes;
	uint64_t announced_before;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ztest_test_skip();
	}

	tc_reset(1, false);

	/* Announce once first. With a floor longer than a tick, the first arm
	 * is already floored, and nothing would be left to observe.
	 */
	tc_now = tc_fire;
	tc_fire = tc_now + tc_reload_period;
	timer_core_announce();

	/* Run the counter well past the deadline without announcing, so every
	 * request below is already due. The requests differ from each other,
	 * because a repeat of the programmed deadline returns early, which is a
	 * different path.
	 */
	tc_now += 40U * (uint64_t)TIMER_CORE_CYC_PER_TICK;
	writes = tc_arm_writes;

	for (uint32_t i = 0; i < 20U; i++) {
		sys_clock_set_timeout(1U + i, false);
	}

	zassert_equal(tc_arm_writes - writes, 1,
		      "a storm of %u set_timeout() calls wrote the hardware %lu times", 20U,
		      tc_arm_writes - writes);
	zassert_true(tc_fire > tc_now, "the pending fire was pushed into the past");

	/* The interrupt arrives, and announcing it lets the next arm through. */
	announced_before = tc_announced;
	tc_now = tc_fire;
	tc_fire = tc_now + tc_reload_period;
	timer_core_announce();

	zassert_true(tc_announced > announced_before, "the floored reload announced nothing");
	tc_check_baseline();
	zassert_equal(timer_core_last_tick, tc_ref_ticks(tc_now), "the storm lost time");

	writes = tc_arm_writes;
	sys_clock_set_timeout(1, false);
	zassert_equal(tc_arm_writes - writes, 1, "the hardware stayed shut after the announce");
#endif
}

ZTEST(timer_core, test_conversions_at_the_ceiling)
{
	/* The round trip above covers the first ticks, where the arithmetic has
	 * room to spare. This checks the arm ceiling, TIMER_CORE_MAX_SPAN_TICKS,
	 * where it has none, at several baseline phases.
	 */
	tc_reset(1, false);

	for (uint32_t phase = 0; phase < 16U; phase++) {
		timer_core_ticks_t cap = TIMER_CORE_MAX_SPAN_TICKS;
		timer_core_cycles_t at_cap;
		timer_core_cycles_t below;

		zassert_true(cap > 1U, "the arm ceiling is %llu ticks", (unsigned long long)cap);

		at_cap = timer_core_span_cycles(cap);
		below = timer_core_span_cycles(cap - 1U);

		zassert_true(at_cap > below, "the span to tick %llu is not past tick %llu",
			     (unsigned long long)cap, (unsigned long long)(cap - 1U));
		zassert_equal(timer_core_ticks_in(at_cap), cap,
			      "the span to the %llu-tick ceiling reads back as %llu",
			      (unsigned long long)cap,
			      (unsigned long long)timer_core_ticks_in(at_cap));
		zassert_equal(timer_core_ticks_in(at_cap - 1U), cap - 1U,
			      "a cycle short of the ceiling reads back as %llu",
			      (unsigned long long)timer_core_ticks_in(at_cap - 1U));

		/* Past the counter's range, the conversion saturates instead of
		 * wrapping to a small count.
		 */
		zassert_true(timer_core_ticks_in(
				     (timer_core_cycles_t)TIMER_CORE_MAX_UNANNOUNCED_CYCLES) >=
				     timer_core_ticks_in(at_cap),
			     "the widest span the counter resolves reads back as %llu, "
			     "under the %llu the ceiling gives",
			     (unsigned long long)timer_core_ticks_in(
				     (timer_core_cycles_t)TIMER_CORE_MAX_UNANNOUNCED_CYCLES),
			     (unsigned long long)timer_core_ticks_in(at_cap));

		timer_core_advance_baseline(1U + (phase % 7U));
	}
}

ZTEST(timer_core, test_arm_zero_ticks)
{
	/* The kernel asks for zero ticks when a timeout is already due. The
	 * timer must still be armed, and not in the past.
	 */
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ztest_test_skip();
	}

	tc_reset(1, false);
	sys_clock_set_timeout(0, false);

	zassert_not_equal(tc_fire, TC_NEVER, "a zero-tick timeout left the timer unarmed");
	zassert_true(tc_fire >= tc_now, "a zero-tick timeout armed %llu cycles in the past",
		     (unsigned long long)(tc_now - tc_fire));
}

#if !defined(CONFIG_64BIT) && !defined(CONFIG_NO_OPTIMIZATIONS)
/*
 * Divisors reaching every form of the reciprocal, at least twice each: with
 * and without bias, and the short and long timer_core_xprod() forms. The
 * powers of two only go to the run-time path, which turns them into a shift.
 */
static const uint32_t tc_divisors[] = {
	3U, 7U, 100U, 1000U, 14318U, 42954U, 100000U, 14318180U, 42954540U,
	1000000007U, 0x80000001U, 0xffffffffU,
	1U, 2U, 1024U, 32768U, 0x80000000U,
};

static void tc_check_div(uint64_t n, uint32_t b)
{
	if (!IS_POWER_OF_TWO(b)) {
		zassert_equal(timer_core_div_const(n, b), n / b,
			      "timer_core_div_const(%llu, %u) gives %llu, want %llu",
			      (unsigned long long)n, b,
			      (unsigned long long)timer_core_div_const(n, b),
			      (unsigned long long)(n / b));
	}
#if !TIMER_CORE_RATE_IS_CONSTANT
	struct timer_core_recip r;

	timer_core_recip_init(&r, b);
	zassert_equal(timer_core_recip_div(n, &r), n / b,
		      "timer_core_recip_div(%llu) for %u gives %llu, want %llu",
		      (unsigned long long)n, b, (unsigned long long)timer_core_recip_div(n, &r),
		      (unsigned long long)(n / b));
#endif
}
#endif

ZTEST(timer_core, test_reciprocal_division)
{
	/* The reciprocal against plain division, over the whole 64-bit range.
	 * b is a variable here, so the derivation runs at run time: this checks
	 * the math, not the constant folding. The edge dividends are where a
	 * rounding error would show first.
	 */
#if defined(CONFIG_64BIT) || defined(CONFIG_NO_OPTIMIZATIONS)
	/* Plain division, no reciprocal. */
	ztest_test_skip();
#else
	tc_rand_state = 20260927;

	for (size_t i = 0; i < ARRAY_SIZE(tc_divisors); i++) {
		uint32_t b = tc_divisors[i];
		uint64_t top = (UINT64_MAX / b) * b;
		const uint64_t edges[] = {
			0U, 1U, b - 1U, b, (uint64_t)b + 1U,
			UINT32_MAX, (uint64_t)UINT32_MAX + 1U, (uint64_t)UINT32_MAX + 2U,
			1ULL << 63, top - 1U, top, UINT64_MAX - 1U, UINT64_MAX,
		};

		for (size_t k = 0; k < ARRAY_SIZE(edges); k++) {
			tc_check_div(edges[k], b);
		}

		/* Random dividends at every magnitude. */
		for (uint32_t k = 0; k < 2000U; k++) {
			uint64_t n = ((uint64_t)tc_rand(UINT32_MAX) << 32) | tc_rand(UINT32_MAX);

			tc_check_div(n >> tc_rand(64U), b);
		}
	}
#endif
}

/* Print what the core derived for this scenario, so a failure log shows the
 * configuration without looking up tests.yaml.
 */
static void *tc_print_config(void)
{
	/* Initialize the core first, so the precomputed values exist. */
	tc_reset(1, false);

	TC_PRINT("timer core configuration:\n");
	TC_PRINT("  backend                            %s\n", TC_BACKEND_NAME);
	TC_PRINT("  CONFIG_TICKLESS_KERNEL             %d\n", IS_ENABLED(CONFIG_TICKLESS_KERNEL));
	TC_PRINT("  CONFIG_64BIT                       %d\n", IS_ENABLED(CONFIG_64BIT));
	TC_PRINT("  CONFIG_SYS_CLOCK_TICKS_PER_SEC     %d\n", CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	TC_PRINT("  CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC %d\n", CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	TC_PRINT("  TIMER_CORE_CYCLES_PER_SEC          %llu\n",
		 (unsigned long long)TIMER_CORE_CYCLES_PER_SEC);
	TC_PRINT("  TIMER_CORE_RATE_IS_CONSTANT        %d\n", TIMER_CORE_RATE_IS_CONSTANT);
	TC_PRINT("  TIMER_CORE_TICK_IS_WHOLE           %d\n", TIMER_CORE_TICK_IS_WHOLE);
	TC_PRINT("  TIMER_CORE_CYC_PER_TICK            %llu\n",
		 (unsigned long long)TIMER_CORE_CYC_PER_TICK);
	TC_PRINT("  TIMER_CORE_COUNTER_WIDTH           %d\n", TIMER_CORE_COUNTER_WIDTH);
	TC_PRINT("  TIMER_CORE_COUNTER_MASK            0x%llx\n",
		 (unsigned long long)TIMER_CORE_COUNTER_MASK);
	TC_PRINT("  TIMER_CORE_COUNTER_SAFE_SPAN       %llu\n",
		 (unsigned long long)TIMER_CORE_COUNTER_SAFE_SPAN);
	TC_PRINT("  TIMER_CORE_MAX_UNANNOUNCED_CYCLES  %llu\n",
		 (unsigned long long)TIMER_CORE_MAX_UNANNOUNCED_CYCLES);
	TC_PRINT("  TIMER_CORE_MAX_ARM_CYCLES          %llu\n",
		 (unsigned long long)TIMER_CORE_MAX_ARM_CYCLES);
	TC_PRINT("  TIMER_CORE_ALARM_MIN_CYCLES        %llu\n",
		 (unsigned long long)TIMER_CORE_ALARM_MIN_CYCLES);
	TC_PRINT("  TIMER_CORE_ALARM_MAX_CYCLES        %llu\n",
		 (unsigned long long)TIMER_CORE_ALARM_MAX_CYCLES);
#if defined(TIMER_CORE_BACKEND_COMPARE_EXACT)
	TC_PRINT("  TIMER_CORE_ALARM_LEAD_CYCLES       %llu\n",
		 (unsigned long long)TIMER_CORE_ALARM_LEAD_CYCLES);
#endif
	TC_PRINT("  TIMER_CORE_MAX_SPAN_TICKS          %llu\n",
		 (unsigned long long)TIMER_CORE_MAX_SPAN_TICKS);
	TC_PRINT("  TIMER_CORE_MAX_TICKS               %llu\n",
		 (unsigned long long)TIMER_CORE_MAX_TICKS);
#if !TIMER_CORE_TICK_IS_WHOLE
	TC_PRINT("  TIMER_CORE_MAX_CONVERTIBLE_CYCLES  %llu\n",
		 (unsigned long long)TIMER_CORE_MAX_CONVERTIBLE_CYCLES);
#endif
	TC_PRINT("  TC_WIDTH                           %d\n", TC_WIDTH);
	TC_PRINT("  TC_ARMS_PER_TICK                   %llu\n",
		 (unsigned long long)TC_ARMS_PER_TICK);

	return NULL;
}

ZTEST_SUITE(timer_core, NULL, tc_print_config, NULL, NULL, NULL);

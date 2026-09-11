/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Unit tests for the generic system-timer core
 *
 * The core is an implementation header, so it is compiled into this test the
 * way a driver compiles it: a simulated counter stands in for hardware, and the
 * two kernel entry points it calls are mocked. Its announce baseline is then
 * visible, which is the point of testing it here rather than through a driver.
 * The two conversion directions have to agree with each other, and where they
 * do not the damage is invisible from outside: announced time still comes out
 * right, paid for with an interrupt that announces nothing.
 *
 * One build covers one configuration, the core being a set of file-static
 * definitions of the global sys_clock_* entry points. The scenarios in
 * tests.yaml walk the backends, the counter widths and the rate ratios.
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/timer/system_timer.h>

/* Counter width under test. */
#ifndef TC_WIDTH
#define TC_WIDTH 32
#endif

/* Low TC_WIDTH bits, the part of the count the hardware register holds. */
#define TC_MASK (UINT64_MAX >> (64 - TC_WIDTH))

#if TC_WIDTH <= 32
typedef uint32_t tc_hw_t;
#else
typedef uint64_t tc_hw_t;
#endif

/*
 * Simulated hardware.
 *
 * tc_now is the absolute cycle count since the simulation started and is never
 * truncated; the driver hooks return it masked to TC_WIDTH, which is what a
 * narrow counter gives a driver and what the core has to put the high bits back
 * onto. tc_fire is the absolute cycle the timer next interrupts at.
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
	/* What the register holds. The core has already clamped to it, and a
	 * value past it would be truncated by real hardware into a deadline
	 * nobody asked for.
	 */
	zassert_true(cycles <= TC_ALARM_MAX, "reload of %llu past the %llu the register holds",
		     (unsigned long long)cycles, (unsigned long long)TC_ALARM_MAX);
#endif
#if defined(TC_ALARM_MIN)
	/* Hardware that cannot be armed nearer than its floor. The core bumps a
	 * shorter request up to it rather than handing it over.
	 */
	zassert_true(cycles >= TC_ALARM_MIN, "reload of %llu under the %llu floor",
		     (unsigned long long)cycles, (unsigned long long)TC_ALARM_MIN);
#endif
	tc_arm_writes++;
	/* Programming a reload restarts the counter, and the hardware the core
	 * assumes reloads itself with the same value afterwards.
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
	/* How far ahead the comparator reaches. A target in the far half of the
	 * window is one the counter has passed, which is a different case and
	 * one the backends are allowed to produce.
	 */
	if (ahead <= (TC_MASK >> 1)) {
		zassert_true(ahead <= TC_ALARM_MAX,
			     "compare set %llu cycles out, past the %llu the alarm reaches",
			     (unsigned long long)ahead, (unsigned long long)TC_ALARM_MAX);
	}
#endif

#if defined(TC_BACKEND_COMPARE_EXACT)
	/* Equality only: a value written for the cycle the counter is already
	 * on, or for one it has passed, is missed until the counter comes round
	 * again. The core's verify loop exists to keep that from happening, so
	 * model it rather than smoothing it over.
	 */
	tc_fire = tc_now + (ahead == 0U ? (TC_MASK + 1U) : ahead);
#else
	/* Ordered comparison: a value at or behind the counter fires at once.
	 * "Behind" is the far half of the masked window, the near half being
	 * what the core is allowed to arm into.
	 */
	tc_fire = (ahead > (TC_MASK >> 1)) ? tc_now : tc_now + ahead;
#endif
}

#endif /* TC_BACKEND_RELOAD */

#define TIMER_CORE_COUNTER_WIDTH TC_WIDTH

#if defined(TC_NONMONOTONIC)
#define TIMER_CORE_COUNTER_NONMONOTONIC
#endif

#if defined(TC_NONATOMIC)
#define TIMER_CORE_COUNTER_NONATOMIC
#endif

#if defined(TC_ALARM_MAX)
/* An arming register narrower than the counter, which is the usual shape: the
 * counter is synthesized in software and the alarm is the hardware's own.
 */
#define TIMER_CORE_ALARM_MAX_CYCLES TC_ALARM_MAX
#endif

#if defined(TC_ALARM_MIN)
/* Hardware that will not be armed nearer than this. */
#define TIMER_CORE_ALARM_MIN_CYCLES TC_ALARM_MIN
#endif

/*
 * A counter whose rate the build cannot see. There are two ways in: the kernel
 * reads its own rate at run time, or the driver states a rate of its own that
 * happens to be a variable. They reach the same arithmetic by different
 * declarations, so both are worth a scenario.
 */
#if defined(TC_RUNTIME_RATE)
/*
 * A counter running faster than the kernel's cycle unit, which is the situation
 * a driver stating a rate of its own is in: the kernel cannot read this counter
 * raw, so the core emits no getter and the driver owes it one that scales. A
 * whole multiple keeps the scaling exact, the tick accounting being what is
 * under test here rather than the rounding of a second conversion.
 */
#define TC_DRIVER_MULT 4U
#else
#define TC_DRIVER_MULT 1U
#endif

/* Counter cycles expressed in the unit the kernel reads. */
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
/* Required of a driver stating a rate of its own; the core then emits neither
 * getter. Defined below the include, where timer_core_cycle_get() is available
 * to scale from. Everywhere else the core's own getters stand, which is what
 * puts the range extension under test.
 */
#define TIMER_CORE_HAVE_CYCLE_GET_32
#endif

/*
 * Mocked kernel side. sys_clock_lock() and sys_clock_unlock() are static inline
 * on a uniprocessor build and reduce to arch_irq_lock(), so they are left alone;
 * only these two are real calls out of the core.
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

void sys_clock_announce_locked(uint32_t ticks, k_spinlock_key_t key)
{
	ARG_UNUSED(key);

	tc_announced += ticks;
	tc_announce_calls++;
	if (ticks == 0U) {
		tc_announce_empty++;
	}

	/* What the kernel does next: work out the following deadline and
	 * reprogram. Driven from here so the core sees the call order it sees
	 * in a real system.
	 */
	if (tc_kernel_rearms) {
		sys_clock_set_timeout(tc_next_timeout(), false);
	}
}

#include "system_timer_generic.h"

#if defined(TC_RUNTIME_RATE)
/* The count in the counter's own domain, extended past the hardware wrap by the
 * core, scaled into the unit the kernel reads.
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
 * Arms needed to cross one tick. More than one only where the arming register
 * cannot express a whole tick, and then every arm short of the last announces
 * nothing: correct, and the only way such hardware gets there.
 */
#define TC_ARMS_PER_TICK                                                                           \
	((uint64_t)DIV_ROUND_UP(TIMER_CORE_CYC_PER_TICK, TIMER_CORE_MAX_ARM_CYCLES))

/* Counter rate as the test knows it, independently of how the core derives it. */
#if TC_RATE_IS_VARIABLE
#define TC_CYCLE_RATE ((uint64_t)tc_rate)
#else
#define TC_CYCLE_RATE ((uint64_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#endif

/*
 * Reference conversion, deliberately not the core's: the whole ticks that have
 * started by cycle @p c, counted from cycle zero. Tick n starts at
 * n * rate / ticks_per_sec, so tick n has started once c reaches that, and the
 * count is the floor of the inverse. A rescale re-expresses the counter in the
 * new domain, still measured from tick zero, so this needs no anchor of its own.
 *
 * Split at the counter rate so that c * TC_TICK_RATE is never formed: the long
 * runs here reach cycle counts whose product with the tick rate leaves 64 bits,
 * and a reference that overflows is worse than no reference.
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
	/* The arm path skips the hardware when the deadline it is asked for is
	 * the one already programmed, so a stale deadline left by the previous
	 * test would leave this one unarmed.
	 */
	timer_core_armed_deadline = UINT64_MAX;
	timer_core_catchup = false;
#endif
	timer_core_init();

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
#if defined(TIMER_CORE_BACKEND_RELOAD)
		/* A tickful RELOAD driver owns its period: the core leaves the
		 * hardware alone, so set it free-running at one tick the way
		 * such a driver's init does.
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
 * The baseline sits on a tick position. Checked against the absolute conversion,
 * which the incremental walk the announce path takes does not go through: the
 * two agreeing is what says the walk has not accumulated an error.
 */
static void tc_check_baseline(void)
{
	timer_core_ticks_t ceiling = timer_core_ticks_at_cyc(TIMER_CORE_MAX_UNANNOUNCED_CYCLES);

	zassert_equal(timer_core_last_cycle, timer_core_cyc_at_tick(timer_core_last_tick),
		      "baseline %llu is not the position of tick %llu (%llu)",
		      (unsigned long long)timer_core_last_cycle,
		      (unsigned long long)timer_core_last_tick,
		      (unsigned long long)timer_core_cyc_at_tick(timer_core_last_tick));

	/* The arm ceiling is the span the counter resolves, expressed in ticks,
	 * and has to still be that at the rate in force now. Where it is
	 * derived once rather than on every arm, a rate that changed since is
	 * what leaves it saying more than the counter can deliver.
	 */
	zassert_true(ceiling >= TIMER_CORE_MAX_SPAN_TICKS,
		     "the arm ceiling of %llu ticks reaches past the %llu the counter resolves",
		     (unsigned long long)TIMER_CORE_MAX_SPAN_TICKS, (unsigned long long)ceiling);
}

/*
 * Announced time is exactly what the counter says, with nothing lost and
 * nothing invented. Holds between announces too, sys_clock_elapsed() being what
 * the kernel adds to the announced count to read the clock.
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

	/* A tickful kernel does not ask, so the clock only moves at the
	 * interrupt: it may lag by what the hardware period has not delivered
	 * yet, and must never run ahead.
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
	 * The invariant the two directions have to meet: the span to the tick n
	 * ahead is exactly n ticks' worth, and a cycle less than that is one
	 * tick less. A conversion pair that disagrees here still keeps time,
	 * and pays for it with an interrupt per tick that announces nothing.
	 *
	 * Checked at every baseline phase, since the remainder is what the two
	 * directions share and it cycles through its whole range.
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
	/* Successive ticks are at least one cycle apart and never move
	 * backwards, whatever the ratio. A tick that shares a cycle with the
	 * next one cannot be announced separately.
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
	/* One tick at a time, which is what a busy system asks for. Every
	 * interrupt must carry a tick: an announce of nothing is an interrupt
	 * the hardware took for no reason, and a conversion pair that disagrees
	 * with itself produces a stream of them while still keeping time.
	 *
	 * Tickless only. A tickful kernel never reprograms, so its period is
	 * whatever the driver loaded once, and where a tick is not a whole
	 * number of cycles no such period can be right.
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
	/* A day, which crosses a 24-bit counter's wrap some thousands of times.
	 * tc_run() checks the announced count against the counter at every
	 * interrupt, so this is about accumulation: a baseline that drifts by a
	 * cycle per wrap shows up here and nowhere else.
	 */
	tc_reset(64, IS_ENABLED(CONFIG_TICKLESS_KERNEL));
	tc_run(TC_CYCLE_RATE * 86400U);

	zassert_equal(timer_core_last_tick, tc_ref_ticks(tc_now), "a day of ticks does not add up");
	zassert_equal(tc_announced, timer_core_last_tick,
		      "announced total does not match the baseline");
}

ZTEST(timer_core, test_clock_read_between_announces)
{
	/* What the kernel sees mid-tick: the announced count plus
	 * sys_clock_elapsed() is the counter, converted, at any cycle and not
	 * only at the ones an interrupt lands on.
	 */
	tc_reset(16, IS_ENABLED(CONFIG_TICKLESS_KERNEL));

	for (uint32_t i = 0; i < 20000U; i++) {
		uint64_t step = 1U + tc_rand(3U * (uint32_t)TIMER_CORE_CYC_PER_TICK);

		/* Stop short of the armed deadline: past it the interrupt is
		 * pending and the clock is legitimately behind.
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
	/* A starved ISR: the counter has run well past the deadline before the
	 * announce happens. The whole span has to come out, not one tick.
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
	/* A deadline further out than one arm reaches is walked to over several
	 * arms, each announcing what it covered. No arm may cross more than the
	 * span the counter resolves, and the clock has to stay exact across the
	 * walk rather than only at the end of it.
	 *
	 * Tickless only: a tickful kernel states no deadline, its timer having
	 * been left to free-run at one tick.
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

		/* An arm that cannot reach the next tick announces nothing, which
		 * is progress in cycles rather than in ticks.
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
	/* A span the driver recovered from elsewhere, wider than the counter
	 * can express. Only whole ticks are announced; the remainder stays for
	 * the next delta.
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
	/* A driver that changes the counter frequency rescales its own count and
	 * tells the core, which re-expresses the announce baseline in the new
	 * domain. The tick count is frequency-independent and must not move
	 * across that, and everything derived from the rate has to follow it.
	 */
#if !TC_RATE_IS_VARIABLE
	/* A rate the build resolved cannot change under the core. */
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

	/* Raised rather than lowered: a faster counter spans less time, so
	 * anything the core worked out from the old rate now says more than the
	 * counter can deliver.
	 */
	tc_rate = to_hz;
	timer_core_rescale(to_hz, from_hz);

	/* What the driver does to its own counter, the core having just placed
	 * the baseline for it. The sub-tick part scales with the rate.
	 */
	tc_now = timer_core_last_cycle + ((frac * to_hz) / from_hz);

	zassert_equal(timer_core_last_tick, tick_at_change,
		      "the rescale moved the clock from %llu to %llu ticks",
		      (unsigned long long)tick_at_change, (unsigned long long)timer_core_last_tick);
	tc_check_baseline();
	zassert_equal(timer_core_last_tick, tc_ref_ticks(tc_now),
		      "the clock does not read the rescaled counter");

	/* And time keeps running, at the rate that is now in force. */
	sys_clock_set_timeout(1, false);
	tc_run(TC_CYCLE_RATE * 5U);
#endif
}

ZTEST(timer_core, test_cycle_get_extends_past_wrap)
{
	/* The kernel's two cycle getters. A counter narrower than the count they
	 * return has to be extended, which the core does from the announce
	 * baseline, and the only interesting place to look is across the point
	 * where the hardware count goes back to zero.
	 */
	uint64_t prev64;
	uint64_t start = 0;
	uint64_t end;

#if TC_WIDTH < 64
	/* Begin a hundredth of a second short of the wrap, so the run crosses
	 * it early and then keeps going well past it.
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
	/* Hardware with a floor cannot be armed for a deadline already due, so
	 * the core arms the floor instead and lets that fire. Programming a
	 * reload restarts the counter, so a stream of set_timeout() calls
	 * arriving faster than the floor, which is what a syscall-heavy thread
	 * resetting its timeslice looks like, would keep pushing the fire point
	 * out and freeze announced time. The core writes the hardware once and
	 * leaves the pending fire alone until it has been announced.
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

	/* Announce once first. Hardware whose floor is longer than a tick has
	 * its very first arm floored, so the storm would otherwise start with a
	 * reload already in flight and nothing left to observe.
	 */
	tc_now = tc_fire;
	tc_fire = tc_now + tc_reload_period;
	timer_core_announce();

	/* Run the counter well past the deadline without announcing it, so
	 * everything asked for below is already due. The deadlines have to
	 * differ from each other: an arm for the deadline already programmed
	 * returns before it reaches the floor, which is a separate short-circuit
	 * and not what is under test here.
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

	/* The fire gets through, and announcing it opens the hardware again. */
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

ZTEST_SUITE(timer_core, NULL, NULL, NULL, NULL, NULL);

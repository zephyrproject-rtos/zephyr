/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic tickless system-timer core
 *
 * This header carries the tick accounting that every tickless system-timer
 * driver would otherwise reimplement by hand: the cycle-to-tick conversion, the
 * announce baseline (last_cycle / last_tick / last_elapsed), the deadline
 * computation and the range clamp. A driver reduces to a few cycle-domain
 * primitives; the tick contract the kernel calls (sys_clock_set_timeout(),
 * sys_clock_elapsed(), sys_clock_cycle_get_32/64()) is emitted here.
 *
 * The file is an implementation header, not a normal declaration header: it
 * *defines* the global sys_clock_* entry points, so it must be included exactly
 * once, by the one chosen system-timer driver, after that driver has provided
 * its constants and primitives.
 *
 * A driver, before the include, provides:
 *
 *   - Exactly one backend feature macro:
 *       TIMER_CORE_BACKEND_COMPARE  free-running counter plus an absolute compare
 *                                register (arm_arch, riscv, hpet, ...)
 *       TIMER_CORE_BACKEND_RELOAD   a counter that fires after a relative delay
 *                                (down-counters, compare-match-reset periodics).
 *                                Assumed to auto-reload: on a non-tickless
 *                                kernel it free-runs from the LOAD set at init
 *                                and is not reprogrammed per tick.
 *
 *   - static inline uint64_t timer_driver_cycle_get(void): the hardware cycle count.
 *     Its rate is TIMER_CORE_CYCLES_PER_SEC (see the knobs below), by default the kernel
 *     system clock rate. It may wrap at the counter width declared by
 *     TIMER_CORE_CYCLES_WIDTH (the core masks deltas to that width), so a narrow
 *     free-running counter is used as-is, without software extension.
 *
 *   - the arming primitive for the chosen backend:
 *       COMPARE: static inline void timer_driver_set_compare(uint32_t/uint64_t cycles)
 *                Fire an interrupt when the counter reaches @p cycles, a
 *                full-width cycle count. The argument width is the driver's: a
 *                64-bit comparator takes uint64_t; a 32-bit one may take uint32_t
 *                (or mask a uint64_t argument) to drop the value to its
 *                comparator width. Must not lose the interrupt for an
 *                already-past target.
 *       RELOAD:  static inline void timer_driver_set_reload(uint32_t/uint64_t cycles)
 *                Fire an interrupt after @p cycles more cycles. The core has
 *                already clamped @p cycles to [TIMER_CORE_MIN_DELAY,
 *                TIMER_CORE_CYCLES_MAX]. The argument width is the driver's:
 *                uint32_t suits a counter whose TIMER_CORE_CYCLES_MAX fits 32
 *                bits (the usual case); a wider counter may take uint64_t and
 *                raise TIMER_CORE_CYCLES_MAX to match. The core does not narrow
 *                the value, so the two must be kept coherent.
 *
 * Optional knobs (sensible defaults provided):
 *
 *   - TIMER_CORE_CYCLES_PER_SEC: rate, in Hz, of the counter timer_driver_cycle_get() reads,
 *     from which the core derives the cycles per tick. Defaults to the kernel
 *     system clock rate; a driver whose counter runs at another rate (prescaled,
 *     or a fixed frequency) overrides it. With a build-time-constant rate the
 *     derived cycles-per-tick is a constant (the tick math divides by it on the
 *     announce path); with a run-time rate
 *     (CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) the core precomputes it once
 *     in timer_core_init(), so that path never divides the rate twice. A driver
 *     that needs cycles-per-tick as a compile-time constant elsewhere may
 *     instead define TIMER_CORE_CYC_PER_TICK directly.
 *   - TIMER_CORE_CYCLES_WIDTH: width, in bits, of the cycle counter (1..64). Defaults to
 *     the native register width, which suits any counter at least that wide;
 *     a narrower counter, or a wide counter to be treated as narrower, sets its
 *     own value. The core masks every delta to this width and never arms a
 *     deadline more than TIMER_CORE_CYCLES_MAX ahead.
 *   - TIMER_CORE_CYCLES_MAX: furthest deadline armed ahead of the last announce. Defaults
 *     to half the counter span (the upper half is IRQ-latency headroom), or a
 *     quarter span when TIMER_CORE_COUNTER_NONMONOTONIC is set.
 *   - TIMER_CORE_64BIT_CYCLES: define to also emit sys_clock_cycle_get_64().
 *   - TIMER_CORE_HAVE_CYCLE_GET_32 / _64: define (and provide your own
 *     sys_clock_cycle_get_32 / _64 before the include) to suppress the core's,
 *     e.g. when the raw counter needs scaling.
 *   - TIMER_CORE_CYCLE_GET_NONATOMIC: define when timer_driver_cycle_get() is not a
 *     single atomic read but a synthesized count (built from a shared software
 *     accumulator the ISR and reprogram paths also touch). The core then reads
 *     it under the clock lock in sys_clock_cycle_get_32/64() instead of raw.
 *   - TIMER_CORE_MIN_DELAY (RELOAD): reload floor, in cycles.
 *   - TIMER_CORE_COUNTER_NONMONOTONIC: define if the counter may momentarily read
 *     behind a value already observed (e.g. a global timer under QEMU SMP); the
 *     core then treats a backwards read as no elapse instead of a huge jump.
 *
 * The driver completes with its IRQ handler (a hardware acknowledge, then
 * timer_core_announce()), its init (connect the IRQ, then timer_core_init()) and,
 * on SMP, an smp_timer_init() that primes the per-CPU timer via
 * timer_core_smp_prime().
 */

#ifndef ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_
#define ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#if defined(TIMER_CORE_BACKEND_COMPARE) == defined(TIMER_CORE_BACKEND_RELOAD)
#error "define exactly one of TIMER_CORE_BACKEND_COMPARE / TIMER_CORE_BACKEND_RELOAD"
#endif

/*
 * Cycles per second of the counter that timer_driver_cycle_get() reads. Defaults to
 * the kernel system clock rate; a driver whose counter runs at another rate (a
 * prescaled or fixed-frequency source) overrides it.
 */
#if !defined(TIMER_CORE_CYCLES_PER_SEC)
#define TIMER_CORE_CYCLES_PER_SEC sys_clock_hw_cycles_per_sec()
#endif

/*
 * Cycles per kernel tick, derived from the rate so a driver normally does not
 * define it. With a build-time-constant rate the division folds, which matters
 * because the tick math divides by it on the announce path. When the rate is
 * only known at run time (CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) it is
 * precomputed once in timer_core_init() into a variable, so that path never
 * divides the rate twice. A driver that needs cycles-per-tick as a compile-time
 * constant elsewhere (e.g. a preprocessor test) may define it directly instead.
 */
#if !defined(TIMER_CORE_CYC_PER_TICK)
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
static uint32_t timer_core_cyc_per_tick;
#define TIMER_CORE_CYC_PER_TICK timer_core_cyc_per_tick
#define TIMER_CORE_PRECOMPUTE_CYC_PER_TICK
#else
#define TIMER_CORE_CYC_PER_TICK \
	((uint32_t)(TIMER_CORE_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC))
/*
 * The whole tick math divides by this, so a counter rate below the tick rate
 * (or a mis-set TIMER_CORE_CYCLES_PER_SEC) that rounds it to zero must be caught. When the
 * default TIMER_CORE_CYCLES_PER_SEC is a build constant, catch it at build time. It is not
 * a constant with CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE (a runtime
 * variable that can even change later), so that case, like the runtime-frequency
 * case above, is checked once in timer_core_init() where the value is known. A
 * driver that defines its own TIMER_CORE_CYC_PER_TICK owns that check.
 */
#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#define TIMER_CORE_CHECK_CYC_PER_TICK_AT_INIT
#else
BUILD_ASSERT(TIMER_CORE_CYC_PER_TICK != 0, "timer counter rate is below the tick rate");
#endif
#endif
#endif

/*
 * Default to the native register width: the masked delta then divides in a
 * single register, and a counter at least this wide never wraps within the
 * scheduling range. A narrower counter overrides TIMER_CORE_CYCLES_WIDTH with its width.
 *
 * A driver whose counter is genuinely wider than the native register (a 64-bit
 * counter on a 32-bit CPU) sets TIMER_CORE_CYCLES_WIDTH to that width to use the
 * counter's full range. It must not be forced wider than the counter really is:
 * a 64-bit mask on a 32-bit counter underflows once the baseline passes 2^32.
 */
#if !defined(TIMER_CORE_CYCLES_WIDTH)
#define TIMER_CORE_CYCLES_WIDTH __LONG_WIDTH__
#endif

/* Wrap mask for the counter. */
#define TIMER_CORE_CYCLES_MASK (UINT64_MAX >> (64 - TIMER_CORE_CYCLES_WIDTH))

/*
 * Furthest deadline that may be armed ahead of the last announce.
 *
 * Half the span by default: the upper half is headroom so a late announce
 * (IRQ latency on a maximum-length arm) still yields an in-range delta rather
 * than wrapping.
 *
 * With backwards-read detection (TIMER_CORE_COUNTER_NONMONOTONIC) the upper half
 * is instead the range that flags a backwards read, so it can no longer double
 * as latency headroom. Arming then reaches only a quarter span ahead, leaving
 * the second quarter as the headroom: a late announce of a maximum arm stays
 * below the half-span backwards threshold, while a genuine backwards read still
 * lands in the upper half. A driver may override TIMER_CORE_CYCLES_MAX.
 */
#ifndef TIMER_CORE_CYCLES_MAX
#ifdef TIMER_CORE_COUNTER_NONMONOTONIC
#define TIMER_CORE_CYCLES_MAX (TIMER_CORE_CYCLES_MASK >> 2)
#else
#define TIMER_CORE_CYCLES_MAX (TIMER_CORE_CYCLES_MASK >> 1)
#endif
#endif

/* Reload floor, in cycles (RELOAD only). A driver whose hardware needs a larger
 * minimum programmable delay overrides it.
 */
#ifndef TIMER_CORE_MIN_DELAY
#define TIMER_CORE_MIN_DELAY 1
#endif

/*
 * Announce baseline, private to this translation unit.
 *
 * last_cycle is the cycle count of the most recent announce, held tick-aligned
 * (an exact multiple of TIMER_CORE_CYC_PER_TICK above the init baseline). Its low
 * TIMER_CORE_CYCLES_WIDTH bits track the hardware counter, so masking a delta against it is
 * wrap-correct even for a counter narrower than 64 bits. last_elapsed is the
 * tick count most recently reported to the kernel via sys_clock_elapsed().
 */
static uint64_t timer_core_last_cycle;
static uint64_t timer_core_last_tick;
static uint32_t timer_core_last_elapsed;

#if defined(TIMER_CORE_BACKEND_RELOAD)
/* A catch-up reload (floored at TIMER_CORE_MIN_DELAY because the deadline is
 * already due, or because the un-announced span is about to overrun
 * TIMER_CORE_CYCLES_MAX) is in flight. Programming a reload restarts the
 * counter, so a stream of set_timeout() calls arriving faster than the floor
 * would rewrite the reload before it can expire and postpone the announce
 * indefinitely; while this is set, timer_core_arm() leaves the hardware alone
 * so the pending fire gets through. Cleared by the announce.
 */
static bool timer_core_catchup;
#endif

/* Tick-aligned deadline currently armed in the hardware, or UINT64_MAX when
 * nothing is known to be armed (initially, and after each announce, which
 * consumes the programmed deadline). timer_core_arm() programs the hardware
 * only when the deadline actually moves: the kernel re-evaluates its earliest
 * timeout on every add and abort (a timeslice reset is one of each), so
 * back-to-back calls usually land on the same tick boundary, and rewriting
 * the timer for those is pure churn. For a RELOAD backend the rewrite is
 * worse than churn: it restarts the counter, so a sustained stream of
 * rewrites (e.g. timeslice resets from a syscall-heavy thread) keeps any
 * period from ever completing and postpones the announce for the duration of
 * the stream.
 */
static uint64_t timer_core_armed_deadline = UINT64_MAX;

/* Whole ticks elapsed since the last announce, from the current counter. */
static inline uint32_t timer_core_delta_ticks(void)
{
	uint64_t delta =
		(timer_driver_cycle_get() - timer_core_last_cycle) & TIMER_CORE_CYCLES_MASK;

#ifdef TIMER_CORE_COUNTER_NONMONOTONIC
	/*
	 * A counter that can momentarily read behind a value already observed
	 * (e.g. a global timer seen from another CPU under QEMU SMP) produces a
	 * near-full-span masked delta. Arming is capped at TIMER_CORE_CYCLES_MAX (a quarter
	 * span here), so a legitimate delta, even a late one, stays in the lower
	 * half; a delta in the upper half is therefore a backwards read and is
	 * reported as no elapse rather than a spurious huge jump.
	 */
	if (delta > (TIMER_CORE_CYCLES_MASK >> 1)) {
		return 0;
	}
#endif

#if TIMER_CORE_CYCLES_WIDTH <= 32
	/* The masked delta fits in a 32-bit register: divide cheaply. */
	return (uint32_t)delta / TIMER_CORE_CYC_PER_TICK;
#else
	return delta / TIMER_CORE_CYC_PER_TICK;
#endif
}

/* Program the timer for a tick-aligned deadline `ticks` out from the last
 * reported elapsed. Called with the system clock lock held.
 */
static void timer_core_arm(uint32_t ticks)
{
	uint64_t deadline_tick = timer_core_last_tick + timer_core_last_elapsed + ticks;

	if (deadline_tick == timer_core_armed_deadline) {
		return;
	}
	timer_core_armed_deadline = deadline_tick;

#if defined(TIMER_CORE_BACKEND_COMPARE)
	/*
	 * Absolute, tick-aligned deadline. last_cycle is linear (its low
	 * TIMER_CORE_CYCLES_WIDTH bits track the counter), and a narrow-register driver
	 * truncates the value to its comparator width when it writes it, so a
	 * linear deadline is correct for both wide and narrow counters.
	 */
	uint64_t deadline = deadline_tick * TIMER_CORE_CYC_PER_TICK;

	if ((deadline - timer_core_last_cycle) > TIMER_CORE_CYCLES_MAX) {
		deadline = timer_core_last_cycle + TIMER_CORE_CYCLES_MAX;
	}
	timer_driver_set_compare(deadline);
#else /* TIMER_CORE_BACKEND_RELOAD */
	/*
	 * Relative reload: the cycles from the last announce to the tick-aligned
	 * deadline, minus what has already elapsed since then. Both terms are in
	 * the counter's cycle domain (the elapsed part masked to TIMER_CORE_CYCLES_WIDTH),
	 * so this stays correct across a counter wrap, and a starved ISR (elapsed
	 * past the deadline) yields a non-positive value pulled up to the floor.
	 */
	uint64_t want = ((uint64_t)timer_core_last_elapsed + ticks) * TIMER_CORE_CYC_PER_TICK;
	uint64_t done = (timer_driver_cycle_get() - timer_core_last_cycle) & TIMER_CORE_CYCLES_MASK;
	int64_t rel = (int64_t)(want - done);

	/*
	 * Clamp against the budget left before the baseline would be TIMER_CORE_CYCLES_MAX
	 * behind, not against TIMER_CORE_CYCLES_MAX alone. Re-arming restarts the counter, so
	 * without accounting `done` a stream of set_timeout() calls with no
	 * intervening announce would push the fire point out indefinitely and let
	 * `done` grow past TIMER_CORE_CYCLES_MASK, wrapping the masked delta and silently
	 * losing the elapsed span. Once `done` reaches TIMER_CORE_CYCLES_MAX the budget goes
	 * non-positive and the MIN_DELAY floor forces a near-immediate fire, hence
	 * an announce that resets the baseline. This keeps the same invariant the
	 * COMPARE path gets from clamping its absolute deadline.
	 */
	if (rel > (int64_t)TIMER_CORE_CYCLES_MAX - (int64_t)done) {
		rel = (int64_t)TIMER_CORE_CYCLES_MAX - (int64_t)done;
	}
	if (rel < (int64_t)TIMER_CORE_MIN_DELAY) {
		/*
		 * The announce is due (or overdue): fire as soon as the floor
		 * allows. If a floored reload is already in flight, leave it
		 * to expire rather than restart the counter, or a set_timeout()
		 * stream arriving faster than the floor (e.g. timeslice resets
		 * from a syscall-heavy thread) would push the fire point out
		 * forever and freeze announced time for the storm's duration.
		 * No incoming deadline can need to fire sooner than the floor
		 * anyway.
		 */
		if (timer_core_catchup) {
			return;
		}
		timer_core_catchup = true;
		rel = TIMER_CORE_MIN_DELAY;
	}
	timer_driver_set_reload((uint64_t)rel);
#endif
}

void sys_clock_set_timeout(uint32_t ticks)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}
	timer_core_arm(ticks);
}

uint32_t sys_clock_elapsed(void)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	timer_core_last_elapsed = timer_core_delta_ticks();
	return timer_core_last_elapsed;
}

/* Account the whole ticks elapsed since the last announce, rearm (tickful only;
 * tickless rearms from the kernel's reprogram after the announce) and announce,
 * with the clock lock already held. @p key is the key returned by the driver's
 * sys_clock_lock() and is consumed here. Use this when the driver must do work
 * that has to be atomic with the announce (e.g. committing a wrap into its cycle
 * counter) before handing off.
 */
static void timer_core_announce_from(k_spinlock_key_t key)
{
	uint32_t dticks = timer_core_delta_ticks();

	timer_core_last_cycle += (uint64_t)dticks * TIMER_CORE_CYC_PER_TICK;
	timer_core_last_tick += dticks;
	timer_core_last_elapsed = 0;
	/* The programmed deadline is consumed (or obsolete): the kernel decides
	 * the next one after the announce, and it must reach the hardware even
	 * if it lands on the same tick.
	 */
	timer_core_armed_deadline = UINT64_MAX;
#if defined(TIMER_CORE_BACKEND_RELOAD)
	timer_core_catchup = false;
#endif

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
#if defined(TIMER_CORE_BACKEND_COMPARE)
		/* Re-arm the comparator one tick out. A RELOAD counter reloads
		 * itself from the LOAD set at init, so it needs nothing here.
		 */
		timer_core_arm(1);
#endif
	}

	sys_clock_announce_locked(dticks, key);
}

/* Read the counter, account the whole ticks elapsed since the last announce,
 * rearm and announce. The driver ISR body reduces to a hardware acknowledge
 * followed by this call. inline so a driver that instead uses
 * timer_core_announce_from() directly does not trip -Wunused-function.
 */
static inline void timer_core_announce(void)
{
	timer_core_announce_from(sys_clock_lock());
}

/*
 * Full-width cycle count reconstructed from the announce baseline (which carries
 * the extended upper bits) plus the masked forward delta, taken under the clock
 * lock. The lock is needed when either the read cannot be trusted atomically:
 *
 *   - timer_driver_cycle_get() is a synthesized count, not a single register read
 *     (TIMER_CORE_CYCLE_GET_NONATOMIC): the lock serialises it against the ISR and
 *     reprogram paths that update the same software state; or
 *   - the full 64-bit baseline is read on a narrower counter (the getter below
 *     needs all 64 bits, which is not a single load on a 32-bit CPU).
 *
 * A narrow counter feeding only the 32-bit getter does NOT need this: that path
 * reconstructs from the baseline's low word, a single atomic load, so it stays
 * lock-free (see sys_clock_cycle_get_32()). Keeping the lock out of the 32-bit
 * getter matters because it is on the thread-usage timestamp hot path.
 */
#if defined(TIMER_CORE_CYCLE_GET_NONATOMIC) || \
	(defined(TIMER_CORE_64BIT_CYCLES) && TIMER_CORE_CYCLES_WIDTH < 64)
static inline uint64_t timer_core_cycle_extend_locked(void)
{
	k_spinlock_key_t key = sys_clock_lock();
	uint64_t delta =
		(timer_driver_cycle_get() - timer_core_last_cycle) & TIMER_CORE_CYCLES_MASK;
	uint64_t ret = timer_core_last_cycle + delta;

	sys_clock_unlock(key);
	return ret;
}
#endif

#if !defined(TIMER_CORE_HAVE_CYCLE_GET_32)
uint32_t sys_clock_cycle_get_32(void)
{
#if defined(TIMER_CORE_CYCLE_GET_NONATOMIC)
	/* Synthesized read: serialise the get and baseline under the lock. */
	return (uint32_t)timer_core_cycle_extend_locked();
#elif TIMER_CORE_CYCLES_WIDTH < 32
	/* Narrow atomic counter: extend from the baseline's low word. Both reads
	 * are single loads, and pairing one baseline read with one counter read
	 * yields the true position regardless of an announce in between, so this
	 * needs no lock.
	 */
	uint32_t base = (uint32_t)timer_core_last_cycle;
	uint32_t delta = ((uint32_t)timer_driver_cycle_get() - base) &
			 (uint32_t)TIMER_CORE_CYCLES_MASK;

	return base + delta;
#else
	return (uint32_t)timer_driver_cycle_get();
#endif
}
#endif

#if defined(TIMER_CORE_64BIT_CYCLES) && !defined(TIMER_CORE_HAVE_CYCLE_GET_64)
uint64_t sys_clock_cycle_get_64(void)
{
#if defined(TIMER_CORE_CYCLE_GET_NONATOMIC) || TIMER_CORE_CYCLES_WIDTH < 64
	return timer_core_cycle_extend_locked();
#else
	return timer_driver_cycle_get();
#endif
}
#endif

/* Rescale the announce baseline from one cycle rate to another. A driver that
 * changes the timer frequency at runtime calls this (after rescaling its own
 * cycle counter) so the tick accounting stays continuous, without touching the
 * core's private baseline directly. last_tick is a tick count and so is
 * frequency-independent; only the cycle-domain baseline scales.
 */
static inline void timer_core_rescale(uint32_t to_hz, uint32_t from_hz)
{
	ARG_UNUSED(from_hz);
#ifdef TIMER_CORE_PRECOMPUTE_CYC_PER_TICK
	/* The rate just changed, so the precomputed cycles-per-tick is stale. */
	timer_core_cyc_per_tick = to_hz / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#else
	ARG_UNUSED(to_hz);
#endif
	/*
	 * Re-express the announce baseline in the new cycle domain. Deriving it
	 * from last_tick (a frequency-independent tick count) keeps it an exact
	 * multiple of TIMER_CORE_CYC_PER_TICK, the invariant the arm and announce paths rely
	 * on. Scaling the old cycle value directly would leave it a fraction of a
	 * tick off and, on the COMPARE arm, could make (deadline - last_cycle)
	 * underflow. The caller has already rescaled its own cycle counter.
	 */
	timer_core_last_cycle = timer_core_last_tick * (uint64_t)TIMER_CORE_CYC_PER_TICK;
}

/* Prime the calling CPU's timer one tick ahead of the shared baseline. An SMP
 * driver calls this from smp_timer_init() so it need not touch the core's
 * private baseline.
 */
static inline void timer_core_smp_prime(void)
{
#if defined(TIMER_CORE_BACKEND_COMPARE)
	timer_driver_set_compare(timer_core_last_cycle + TIMER_CORE_CYC_PER_TICK);
#else
	timer_driver_set_reload(TIMER_CORE_CYC_PER_TICK);
#endif
}

/* Seed the announce baseline from the current counter and arm the first tick.
 * The driver calls this from its init after connecting the IRQ (and, for a
 * runtime frequency, after setting TIMER_CORE_CYC_PER_TICK).
 */
static inline void timer_core_init(void)
{
#ifdef TIMER_CORE_PRECOMPUTE_CYC_PER_TICK
	/* Rate is known only now (the driver has brought the counter up), so
	 * fix the cycles-per-tick the tick math will divide by.
	 */
	timer_core_cyc_per_tick = TIMER_CORE_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#endif
#if defined(TIMER_CORE_PRECOMPUTE_CYC_PER_TICK) || defined(TIMER_CORE_CHECK_CYC_PER_TICK_AT_INIT)
	/* Runtime-rate cases: TIMER_CORE_CYC_PER_TICK is not a constant expression, so the
	 * non-zero check the constant case gets at build time happens here instead.
	 */
	__ASSERT(TIMER_CORE_CYC_PER_TICK != 0, "timer counter rate is below the tick rate");
#endif
	timer_core_last_tick = timer_driver_cycle_get() / TIMER_CORE_CYC_PER_TICK;
	timer_core_last_cycle = timer_core_last_tick * TIMER_CORE_CYC_PER_TICK;
	timer_core_last_elapsed = 0;

#if defined(TIMER_CORE_BACKEND_RELOAD)
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* A tickful RELOAD driver configures its hardware to auto-reload one
		 * tick and is never reprogrammed (see timer_core_announce_from()), so the
		 * driver owns the period. Arming here would instead program it to the
		 * sub-tick remainder left after seeding the baseline, and that short
		 * value would stick as the permanent period.
		 */
		return;
	}
#endif
	timer_core_arm(1);
}

#endif /* ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_ */

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
 * sys_clock_elapsed()) is emitted here.
 *
 * The file is an implementation header, not a normal declaration header: it
 * *defines* the global sys_clock_* entry points. Any number of drivers can be
 * built on it; each includes it once, after providing its own constants and
 * primitives. A build compiles a single system-timer driver, so those
 * definitions land once.
 *
 * A driver, before the include, provides:
 *
 *   - The backend feature macro:
 *       TIMER_CORE_BACKEND_COMPARE_ORDERED free-running counter plus an absolute
 *                                compare register whose match is an ordered
 *                                comparison: the interrupt fires once the counter
 *                                reaches or has passed the value, so a target
 *                                already in the past fires at once (arm_arch,
 *                                riscv_machine, ...). The usable range is half
 *                                the counter width, which is what
 *                                TIMER_CORE_ALARM_MAX_CYCLES bounds.
 *
 * If arming requires the timer interrupt to be enabled, enable it inside the
 * arming primitive rather than tracking it separately. The core does not model
 * interrupt masking.
 *
 *   - static inline uint32_t/uint64_t timer_driver_cycle_get(void): the hardware cycle
 *     count. Its rate is TIMER_CORE_CYCLES_PER_SEC (see the knobs below), by default the
 *     kernel system clock rate. Return the raw counter, even a narrow one that wraps:
 *     declare its width with TIMER_CORE_COUNTER_WIDTH and the core masks every delta to
 *     that width, so the driver never has to widen the count in software. The return
 *     width is the driver's, as with the arming primitives above: uint32_t for a counter
 *     of TIMER_CORE_COUNTER_WIDTH 32 or less, uint64_t for a wider one. The core narrows
 *     what it reads to the declared width anyway, so returning the register's own type
 *     spares a 32-bit target the widening.
 *
 *   - the arming primitive:
 *       static inline void timer_driver_set_compare(uint32_t/uint64_t cycles)
 *                Write the comparator so an interrupt fires when the counter
 *                reaches @p cycles, a full-width cycle count. The argument width
 *                is the driver's: a 64-bit comparator takes uint64_t; a 32-bit
 *                one may take uint32_t (or mask a uint64_t argument) to drop the
 *                value to its comparator width. A plain register write is all
 *                that is wanted here: the hardware handles a past target itself.
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
 *     in timer_core_init(), so that path never divides the rate twice.
 *     The core derives TIMER_CORE_CYC_PER_TICK from it, which a driver may read
 *     for its own hardware setup but never defines itself.
 *   - TIMER_CORE_CYCLES_PER_SEC_RUNTIME: define when the override above is a
 *     variable rather than a build-time constant, a rate read from the clock
 *     controller at init say. The core then precomputes the cycles per tick the
 *     same way it does for CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME. The
 *     variable must hold its final value before timer_core_init() is called.
 *   - TIMER_CORE_COUNTER_WIDTH: width, in bits, of the count timer_driver_cycle_get()
 *     returns, up to 64. Defaults to the native register width, which suits any
 *     counter at least that wide; a counter of another width, including a 64-bit one
 *     on a 32-bit CPU, states its own. The core masks every delta to this width,
 *     which is what lets a narrow counter be returned raw.
 *   - TIMER_CORE_COUNTER_NONMONOTONIC: define if the counter may momentarily read
 *     behind a value already observed (e.g. a global timer under QEMU SMP); the
 *     core then treats a backwards read as no elapse instead of a huge jump.
 * *   - TIMER_CORE_ALARM_MAX_CYCLES: largest value the arming primitive can express,
 *     nothing more. Defaults to the whole span of TIMER_CORE_COUNTER_WIDTH, the alarm and
 *     the counter usually being the same hardware. Set it where the arming range is
 *     decided by something else: a compare or reload register narrower than the counter,
 *     or a separate device. It is a statement about the hardware, so it carries no
 *     safety margin; the core derives its own from TIMER_CORE_COUNTER_WIDTH.
 * The driver completes with its IRQ handler (a hardware acknowledge, then
 * timer_core_announce()), its init (connect the IRQ, then timer_core_init()),
 * sys_clock_cycle_get_32() and, on SMP, an smp_timer_init() that primes the
 * per-CPU timer via timer_core_smp_prime().
 */

#ifndef ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_
#define ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#if !defined(TIMER_CORE_BACKEND_COMPARE_ORDERED)
#error "define the backend: TIMER_CORE_BACKEND_COMPARE_ORDERED"
#endif

/*
 * Cycles per second of the counter that timer_driver_cycle_get() reads. Defaults to
 * the kernel system clock rate; a driver whose counter runs at another rate (a
 * prescaled or fixed-frequency source) overrides it.
 */
#if !defined(TIMER_CORE_CYCLES_PER_SEC)
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#define TIMER_CORE_CYCLES_PER_SEC sys_clock_hw_cycles_per_sec()
#else
/* The Kconfig symbol rather than sys_clock_hw_cycles_per_sec(), whose expansion
 * carries a cast: both operands here are Kconfig integers, which keeps
 * TIMER_CORE_CYC_PER_TICK usable in a preprocessor test.
 */
#define TIMER_CORE_CYCLES_PER_SEC CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define TIMER_CORE_CYC_PER_TICK_IS_CONSTANT
#endif
#endif

/*
 * Cycles per kernel tick, always derived here from the rate: a driver states the
 * rate and reads this. With a build-time-constant rate the division folds, which
 * matters because the tick math divides by it on the announce path. When the rate
 * is only known at run time (CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME, or a
 * driver rate flagged with TIMER_CORE_CYCLES_PER_SEC_RUNTIME) it is precomputed
 * once in timer_core_init() into a variable, so that path never divides the rate
 * twice.
 */
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(TIMER_CORE_CYCLES_PER_SEC_RUNTIME)
static uint32_t timer_core_cyc_per_tick;
#define TIMER_CORE_CYC_PER_TICK timer_core_cyc_per_tick
#define TIMER_CORE_PRECOMPUTE_CYC_PER_TICK
#else
#define TIMER_CORE_CYC_PER_TICK (TIMER_CORE_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
/*
 * The whole tick math divides by this, so a counter rate below the tick rate
 * (or a mis-set TIMER_CORE_CYCLES_PER_SEC) that rounds it to zero must be caught. When the
 * default TIMER_CORE_CYCLES_PER_SEC is a build constant, catch it at build time. It is not
 * a constant with CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE (a runtime
 * variable that can even change later), so that case, like the runtime-frequency
 * case above, is checked once in timer_core_init() where the value is known.
 */
#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#define TIMER_CORE_CHECK_CYC_PER_TICK_AT_INIT
#else
BUILD_ASSERT(TIMER_CORE_CYC_PER_TICK != 0, "timer counter rate is below the tick rate");
#endif
#endif

/*
 * Default to the native register width: the masked delta then divides in a
 * single register, and a counter at least this wide never wraps within the
 * scheduling range. A narrower counter overrides TIMER_CORE_COUNTER_WIDTH with its width.
 * The width must never be forced wider than the counter really is: a 64-bit
 * mask on a 32-bit counter underflows once the baseline passes 2^32.
 */
#if !defined(TIMER_CORE_COUNTER_WIDTH)
#define TIMER_CORE_COUNTER_WIDTH __LONG_WIDTH__
#endif

/* Wrap mask for the counter. */
#define TIMER_CORE_COUNTER_MASK (UINT64_MAX >> (64 - TIMER_CORE_COUNTER_WIDTH))

/*
 * Type holding a counter value, or a masked delta between two. A counter that
 * fits a 32-bit register gets a 32-bit one, so the deltas and the division on
 * the announce path stay single-register work on a 32-bit CPU. The announce
 * baseline is 64-bit either way: it is what carries the count past the wrap
 * this type is allowed to lose.
 */
#if TIMER_CORE_COUNTER_WIDTH <= 32
typedef uint32_t timer_core_cycles_t;
#else
typedef uint64_t timer_core_cycles_t;
#endif

/*
 * Furthest deadline the arming hardware can express, in cycles.
 *
 * The counter's own width by default, because the counter and the alarm are
 * usually one piece of hardware: a free-running counter matched by a comparator
 * of the same width. Where the arming range is decided by something else, a
 * compare or reload register narrower than the counter or a separate device,
 * the driver overrides this with what that hardware holds.
 *
 * This states a hardware limit and nothing else. The margin the core keeps
 * against a late announce is TIMER_CORE_COUNTER_SAFE_SPAN below, and what the
 * arm path actually honours is the smaller of the two.
 */
#ifndef TIMER_CORE_ALARM_MAX_CYCLES
#define TIMER_CORE_ALARM_MAX_CYCLES TIMER_CORE_COUNTER_MASK
#endif

/*
 * Widest span since the last announce whose masked delta the core can still
 * resolve, derived from the counter width alone.
 *
 * Half the span: the upper half is headroom, so a late announce (IRQ latency on
 * a maximum-length wait) still yields an in-range delta rather than a wrapped
 * one. With backwards-read detection (TIMER_CORE_COUNTER_NONMONOTONIC) the upper
 * half is instead the range that flags a backwards read, so it can no longer
 * double as headroom: the span drops to a quarter and the second quarter becomes
 * the headroom.
 *
 * This is the core's business, not the driver's. A driver states how wide its
 * counter is and how much its arming register holds; what margin those imply is
 * worked out here.
 */
#ifdef TIMER_CORE_COUNTER_NONMONOTONIC
#define TIMER_CORE_COUNTER_SAFE_SPAN (TIMER_CORE_COUNTER_MASK >> 2)
#else
#define TIMER_CORE_COUNTER_SAFE_SPAN (TIMER_CORE_COUNTER_MASK >> 1)
#endif

/*
 * Furthest ahead of the last announce that the core will arm: what the counter
 * can still resolve, or what the alarm can express, whichever binds first.
 */
#define TIMER_CORE_MAX_UNANNOUNCED_CYCLES                                                          \
	MIN((uint64_t)TIMER_CORE_COUNTER_SAFE_SPAN, (uint64_t)TIMER_CORE_ALARM_MAX_CYCLES)

/*
 * Announce baseline, private to this translation unit.
 *
 * last_cycle is the cycle count of the most recent announce, held tick-aligned
 * (an exact multiple of TIMER_CORE_CYC_PER_TICK above the init baseline). Its low
 * TIMER_CORE_COUNTER_WIDTH bits track the hardware counter, so masking a delta against it is
 * wrap-correct even for a counter narrower than 64 bits. last_elapsed is the
 * tick count most recently reported to the kernel via sys_clock_elapsed().
 */
static uint64_t timer_core_last_cycle;
static uint64_t timer_core_last_tick;

/* Counter cycles from @p from to now, masked to the counter width so it stays
 * correct across a wrap. Both terms narrow to the counter's own type first:
 * what that drops from the baseline is the extension the mask discards anyway.
 */
static inline timer_core_cycles_t timer_core_cycles_since(uint64_t from)
{
	return ((timer_core_cycles_t)timer_driver_cycle_get() - (timer_core_cycles_t)from) &
	       (timer_core_cycles_t)TIMER_CORE_COUNTER_MASK;
}

/* Largest tick count the span since an announce can reach.
 *
 * It is bounded by the divisor as much as by the counter width: a 48-bit
 * counter at 100000 cycles per tick tops out near 2.8e9 ticks, which fits 32
 * bits, where the width alone would say otherwise. Evaluating that needs cycles
 * per tick as a preprocessor constant, and TIMER_CORE_CYC_PER_TICK is not one:
 * both arms of TIMER_CORE_CYCLES_PER_SEC carry a cast, which #if rejects. So the
 * division is only done when the counter runs at the kernel rate and that rate
 * is a build constant. Otherwise the width alone decides, which errs towards
 * the wider type.
 */
#if defined(TIMER_CORE_CYC_PER_TICK_IS_CONSTANT)
#define TIMER_CORE_MAX_TICKS (TIMER_CORE_COUNTER_MASK / TIMER_CORE_CYC_PER_TICK)
#else
#define TIMER_CORE_MAX_TICKS TIMER_CORE_COUNTER_MASK
#endif

/* Tick counts internal to the core. When the span above can outgrow the 32-bit
 * kernel interface, they are held wider: nothing bounds that span while no
 * timeout is pending, since the comparator sits as far out as it goes and no
 * announce moves the baseline. Only the two places that hand a count to the
 * kernel clamp, and they use timer_core_ticks_clamp().
 */
#if TIMER_CORE_MAX_TICKS > UINT32_MAX
typedef uint64_t timer_core_ticks_t;
static inline uint32_t timer_core_ticks_clamp(timer_core_ticks_t ticks)
{
	return (uint32_t)MIN(ticks, (timer_core_ticks_t)UINT32_MAX);
}
#else
typedef uint32_t timer_core_ticks_t;
static inline uint32_t timer_core_ticks_clamp(timer_core_ticks_t ticks)
{
	return ticks;
}
#endif
static timer_core_ticks_t timer_core_last_elapsed;

/* Whole ticks elapsed since the last announce, from the current counter. */
static inline timer_core_ticks_t timer_core_delta_ticks(void)
{
	timer_core_cycles_t delta = timer_core_cycles_since(timer_core_last_cycle);

#ifdef TIMER_CORE_COUNTER_NONMONOTONIC
	/*
	 * A counter that can momentarily read behind a value already observed
	 * (e.g. a global timer seen from another CPU under QEMU SMP) produces a
	 * near-full-span masked delta. Arming is capped at TIMER_CORE_ALARM_MAX_CYCLES (a quarter
	 * span here), so a legitimate delta, even a late one, stays in the lower
	 * half; a delta in the upper half is therefore a backwards read and is
	 * reported as no elapse rather than a spurious huge jump.
	 */
	if (delta > (timer_core_cycles_t)(TIMER_CORE_COUNTER_MASK >> 1)) {
		return 0;
	}
#endif

	return delta / TIMER_CORE_CYC_PER_TICK;
}

/* Program the timer for a tick-aligned deadline `ticks` out from the last
 * reported elapsed. Called with the system clock lock held.
 */
static void timer_core_arm(uint32_t ticks)
{
	uint64_t deadline_tick = timer_core_last_tick + timer_core_last_elapsed + ticks;

	/*
	 * Absolute, tick-aligned deadline. last_cycle is linear (its low
	 * TIMER_CORE_COUNTER_WIDTH bits track the counter), and a narrow-register driver
	 * truncates the value to its comparator width when it writes it, so a
	 * linear deadline is correct for both wide and narrow counters.
	 */
	uint64_t deadline = deadline_tick * TIMER_CORE_CYC_PER_TICK;

	if ((deadline - timer_core_last_cycle) > TIMER_CORE_MAX_UNANNOUNCED_CYCLES) {
		deadline = timer_core_last_cycle + TIMER_CORE_MAX_UNANNOUNCED_CYCLES;
	}
	timer_driver_set_compare(deadline);
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	/* Deprecated; the idle-entry hint travels via sys_clock_idle_enter(). */
	ARG_UNUSED(idle);

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
	return timer_core_ticks_clamp(timer_core_last_elapsed);
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
	timer_core_ticks_t dticks = timer_core_delta_ticks();

	timer_core_last_cycle += (uint64_t)dticks * TIMER_CORE_CYC_PER_TICK;
	timer_core_last_tick += dticks;
	timer_core_last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Re-arm the comparator one tick out. */
		timer_core_arm(1);
	}

	/* The baseline above moved by the whole delta, so it stays aligned with
	 * the counter. Only what the kernel is told is clamped to the width of
	 * its interface, and that difference is uptime drift, which is what
	 * sloppy idle allows in the first place.
	 */
	sys_clock_announce_locked(timer_core_ticks_clamp(dticks), key);
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
	timer_core_last_cycle = timer_core_last_tick * TIMER_CORE_CYC_PER_TICK;
}

/* Prime the calling CPU's timer one tick ahead of the shared baseline. An SMP
 * driver calls this from smp_timer_init() so it need not touch the core's
 * private baseline.
 */
static inline void timer_core_smp_prime(void)
{
	timer_driver_set_compare(timer_core_last_cycle + TIMER_CORE_CYC_PER_TICK);
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

	timer_core_arm(1);
}

#endif /* ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_ */

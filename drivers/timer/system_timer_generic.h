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
 * *defines* the global sys_clock_* entry points. Any number of drivers can be
 * built on it; each includes it once, after providing its own constants and
 * primitives. A build compiles a single system-timer driver, so those
 * definitions land once.
 *
 * A driver, before the include, provides:
 *
 *   - Exactly one backend feature macro:
 *       TIMER_CORE_BACKEND_COMPARE_ORDERED free-running counter plus an absolute
 *                                compare register whose match is an ordered
 *                                comparison: the interrupt fires once the counter
 *                                reaches or has passed the value, so a target
 *                                already in the past fires at once (arm_arch,
 *                                riscv_machine, ...). The usable range is half
 *                                the counter width, which is what
 *                                TIMER_CORE_ALARM_MAX_CYCLES bounds.
 *       TIMER_CORE_BACKEND_COMPARE_EXACT   the same hardware shape, but the match
 *                                is on equality only: a target written after the
 *                                counter has passed it is missed for a whole
 *                                counter period (hpet, mips_cp0, ...). The core
 *                                writes such a comparator through a verify loop
 *                                (see timer_core_set_compare()), so the
 *                                driver needs no minimum-delay floor of its own.
 *       TIMER_CORE_BACKEND_RELOAD          a counter that fires after a relative
 *                                delay (down-counters, compare-match-reset
 *                                periodics). Assumed to auto-reload: on a
 *                                non-tickless kernel it free-runs from the LOAD
 *                                set at init and is not reprogrammed per tick.
 *
 * If arming requires the timer interrupt to be enabled, enable it inside the
 * arming primitive rather than tracking it separately. The core does not model
 * interrupt masking.
 *
 *   - static uint32_t/uint64_t timer_driver_cycle_get(void): the hardware cycle
 *     count. Its rate is TIMER_CORE_CYCLES_PER_SEC (see the knobs below), by default the
 *     kernel system clock rate. Return the raw counter, even a narrow one that wraps:
 *     declare its width with TIMER_CORE_COUNTER_WIDTH and the core masks every delta to
 *     that width, so the driver never has to widen the count in software. The return
 *     width is the driver's, as with the arming primitives above: uint32_t for a counter
 *     of TIMER_CORE_COUNTER_WIDTH 32 or less, uint64_t for a wider one. The core narrows
 *     what it reads to the declared width anyway, so returning the register's own type
 *     spares a 32-bit target the widening.
 *
 *   - the arming primitive for the chosen backend:
 *       COMPARE: static void timer_driver_set_compare(uint32_t/uint64_t cycles)
 *                Write the comparator so an interrupt fires when the counter
 *                reaches @p cycles, a full-width cycle count. The argument width
 *                is the driver's: a 64-bit comparator takes uint64_t; a 32-bit
 *                one may take uint32_t (or mask a uint64_t argument) to drop the
 *                value to its comparator width. A plain register write is all
 *                that is wanted here: with COMPARE_ORDERED the hardware handles a
 *                past target itself, and with COMPARE_EXACT the core wraps this
 *                in the verify loop that deals with it.
 *       RELOAD:  static void timer_driver_set_reload(uint32_t/uint64_t cycles)
 *                Fire an interrupt after @p cycles more cycles. The core has
 *                already clamped @p cycles to [TIMER_CORE_ALARM_MIN_CYCLES,
 *                TIMER_CORE_ALARM_MAX_CYCLES]. The argument width is the driver's:
 *                uint32_t suits a counter whose TIMER_CORE_ALARM_MAX_CYCLES fits 32
 *                bits (the usual case); a wider counter may take uint64_t and
 *                raise TIMER_CORE_ALARM_MAX_CYCLES to match. The core does not narrow
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
 *   - TIMER_CORE_COUNTER_NONATOMIC: define when timer_driver_cycle_get() is not a
 *     single atomic read but a synthesized count (built from a shared software
 *     accumulator the ISR and reprogram paths also touch). The core then reads
 *     it under the clock lock in sys_clock_cycle_get_32/64() instead of raw.
 *   - TIMER_CORE_HAVE_CYCLE_GET_32 / _64: define (and provide your own
 *     sys_clock_cycle_get_32 / _64) to suppress the core's, e.g. when the raw counter
 *     needs scaling. timer_core_cycle_get(), available after the include, gives the
 *     full-width count in the counter's own domain to scale from. The 32-bit one is
 *     required of a driver that sets TIMER_CORE_CYCLES_PER_SEC, since a counter
 *     running at a rate of its own is one the kernel cannot read raw. The core then
 *     emits no 64-bit getter either, that being in the domain the driver just
 *     declared foreign; supply one as well to have it.
 *   - TIMER_CORE_ALARM_MAX_CYCLES: largest value the arming primitive can express,
 *     nothing more. Defaults to the whole span of TIMER_CORE_COUNTER_WIDTH, the alarm and
 *     the counter usually being the same hardware. Set it where the arming range is
 *     decided by something else: a compare or reload register narrower than the counter,
 *     or a separate device. It is a statement about the hardware, so it carries no
 *     safety margin; the core derives its own from TIMER_CORE_COUNTER_WIDTH.
 *   - TIMER_CORE_ALARM_MIN_CYCLES (RELOAD): reload floor, in cycles.
 *   - TIMER_CORE_ALARM_LEAD_CYCLES (COMPARE_EXACT): cycles a compare must be ahead of
 *     the counter for the match to be caught. Defaults to 1. Raise it for hardware
 *     that carries the write into the counter's clock domain first.
 * The driver completes with its IRQ handler (a hardware acknowledge, then
 * timer_core_announce()), its init (connect the IRQ, then timer_core_init()) and,
 * on SMP, an smp_timer_init() that primes the per-CPU timer via
 * timer_core_smp_prime().
 */

#ifndef ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_
#define ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#if (defined(TIMER_CORE_BACKEND_COMPARE_ORDERED) + \
	defined(TIMER_CORE_BACKEND_COMPARE_EXACT) + \
	defined(TIMER_CORE_BACKEND_RELOAD)) != 1
#error "define exactly one backend: TIMER_CORE_BACKEND_COMPARE_ORDERED, " \
	"TIMER_CORE_BACKEND_COMPARE_EXACT or TIMER_CORE_BACKEND_RELOAD"
#endif

/*
 * Cycles per second of the counter that timer_driver_cycle_get() reads. Defaults to
 * the kernel system clock rate; a driver whose counter runs at another rate (a
 * prescaled or fixed-frequency source) overrides it.
 */
#if defined(TIMER_CORE_CYCLES_PER_SEC)
/* The rate is the driver's own, which the cycle getter rule further down keys on. */
#define TIMER_CORE_DRIVER_CYCLES_PER_SEC
#endif

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
 * Whether the rate is a build constant, which decides whether what is derived
 * from it below folds or has to be worked out once at init into a variable.
 *
 * Weaker than TIMER_CORE_CYC_PER_TICK_IS_CONSTANT above, which needs a
 * preprocessor constant because its user is an #if: a driver rate the compiler
 * folds but the preprocessor cannot read satisfies this one only.
 */
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) ||                                        \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE) ||                           \
	defined(TIMER_CORE_CYCLES_PER_SEC_RUNTIME)
#define TIMER_CORE_RATE_IS_CONSTANT 0
#else
#define TIMER_CORE_RATE_IS_CONSTANT 1
#endif

/*
 * Whether a tick is a whole number of cycles. Where it is, a tick is
 * TIMER_CORE_CYC_PER_TICK cycles and the conversions are a multiply and a
 * divide by it; where it is not, the announce baseline carries the cycle the
 * rounding left over and both directions go through that.
 *
 * Answered by the preprocessor so that only the form in use is compiled. That
 * needs the rate readable by the preprocessor, which a rate read at run time is
 * not: such a rate takes the general form whatever it turns out to be.
 */
#if defined(TIMER_CORE_CYC_PER_TICK_IS_CONSTANT)
/* Nested rather than &&-ed: #if evaluates neither operand lazily, and a rate
 * read at run time is a function call the preprocessor cannot parse at all.
 */
#if (TIMER_CORE_CYCLES_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC) == 0
#define TIMER_CORE_TICK_IS_WHOLE 1
#else
#define TIMER_CORE_TICK_IS_WHOLE 0
/*
 * Say so: the general form is larger on both the announce and the arm path, and
 * the tick rate is the one term here a configuration can pick. Against a
 * 32768Hz counter, 1024 rather than 1000 costs nothing and drops it.
 *
 * Held at warning severity, since the configuration works: building with
 * warnings as errors, which is what twister does by default, must not fail on
 * it. A toolchain without the GCC diagnostic pragma gets the warning at its own
 * default severity.
 */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wcpp"
#endif
#warning "CONFIG_SYS_CLOCK_TICKS_PER_SEC does not divide the counter rate; a divisor is cheaper"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif
#else
#define TIMER_CORE_TICK_IS_WHOLE 0
#endif

/*
 * Cycles per kernel tick, always derived here from the rate: a driver states the
 * rate and reads this. It is the tick period only where a tick is a whole number
 * of cycles; where it is not, it is the truncated period, which the conversions
 * further down do not use. A rate known only at run time has it worked out once
 * in timer_core_init() into a variable rather than dividing at every use.
 */
/*
 * The tick period split into whole cycles and the fraction left over, and the
 * reciprocal of the counter rate. Everything the hot paths need, so that
 * neither direction divides by the counter rate at run time: a 32-bit target
 * has no instruction for that and would call into libgcc from the scheduler.
 *
 * TIMER_CORE_CYC_REM is below the tick rate by construction, which is what
 * keeps the fractional arithmetic inside 32 bits. TIMER_CORE_CYC_RECIP is
 * floor(ticks_per_sec * 2^32 / rate), below 2^32 because the counter is faster
 * than the tick. Neither is wanted where the hardware divides 64 bits itself.
 */
#if !defined(CONFIG_64BIT)
#define TIMER_CORE_CYC_REM_OF(rate) ((uint32_t)((rate) % CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define TIMER_CORE_CYC_RECIP_OF(rate)                                                              \
	((uint32_t)(((uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC << 32) / (rate)))

/* Largest span the fractional arithmetic above carries: span * CYC_REM has to
 * stay inside 32 bits, and CYC_REM is below the tick rate.
 */
#define TIMER_CORE_MAX_FRAC_SPAN_TICKS (UINT32_MAX / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#endif /* !CONFIG_64BIT */

#if TIMER_CORE_RATE_IS_CONSTANT
#define TIMER_CORE_CYC_PER_TICK (TIMER_CORE_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#if !defined(CONFIG_64BIT)
#define TIMER_CORE_CYC_REM   TIMER_CORE_CYC_REM_OF(TIMER_CORE_CYCLES_PER_SEC)
#define TIMER_CORE_CYC_RECIP TIMER_CORE_CYC_RECIP_OF(TIMER_CORE_CYCLES_PER_SEC)
#endif
/*
 * A counter rate below the tick rate (or a mis-set TIMER_CORE_CYCLES_PER_SEC)
 * rounds this to zero, which the tick math cannot work with. The rate being a
 * build constant, catch it here; the runtime case is checked in
 * timer_core_init() where the value is known.
 */
BUILD_ASSERT(TIMER_CORE_CYC_PER_TICK != 0, "timer counter rate is below the tick rate");
#else
static uint32_t timer_core_cyc_per_tick;
#define TIMER_CORE_CYC_PER_TICK timer_core_cyc_per_tick
#if !defined(CONFIG_64BIT)
static uint32_t timer_core_cyc_rem;
static uint32_t timer_core_cyc_recip;
#define TIMER_CORE_CYC_REM   timer_core_cyc_rem
#define TIMER_CORE_CYC_RECIP timer_core_cyc_recip
#endif
#define TIMER_CORE_PRECOMPUTE_CYC_PER_TICK
#endif

/*
 * Default to the native register width: the masked delta then divides in a
 * single register, and a counter at least this wide never wraps within the
 * scheduling range. A counter of another width states its own, and must state it:
 * a genuine 64-bit counter on a 32-bit CPU would otherwise inherit a 32-bit mask
 * and silently lose any span past 2^32 cycles (a long debugger halt, a low-power
 * sleep). It must never be forced wider than the counter really is either: a
 * 64-bit mask on a 32-bit counter underflows once the baseline passes 2^32.
 */
#if !defined(TIMER_CORE_COUNTER_WIDTH)
#define TIMER_CORE_COUNTER_WIDTH (__SIZEOF_LONG__ * 8)
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
 *
 * A deadline further out than this is walked to over several arms, each
 * announcing what it covered. On a compare backend that walk needs the alarm to
 * reach a whole tick: the deadline is an absolute point clamped to this much
 * past the announce baseline, so once the counter passes it with no tick to
 * announce, the baseline stays put and every re-arm names the same point. A
 * reload is relative to the counter and shortens as it advances, so it has no
 * such floor. Asserted below.
 */
#ifndef TIMER_CORE_ALARM_MAX_CYCLES
#define TIMER_CORE_ALARM_MAX_CYCLES TIMER_CORE_COUNTER_MASK
#endif

/* Reload floor, in cycles (RELOAD only). A driver whose hardware needs a larger
 * minimum programmable delay overrides it.
 */
#ifndef TIMER_CORE_ALARM_MIN_CYCLES
#define TIMER_CORE_ALARM_MIN_CYCLES 1
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
#define TIMER_CORE_COUNTER_WIDTH_SPAN (TIMER_CORE_COUNTER_MASK >> 2)
#else
#define TIMER_CORE_COUNTER_WIDTH_SPAN (TIMER_CORE_COUNTER_MASK >> 1)
#endif

#if TIMER_CORE_TICK_IS_WHOLE
#define TIMER_CORE_COUNTER_SAFE_SPAN TIMER_CORE_COUNTER_WIDTH_SPAN
#else
/*
 * Widest cycle span the tick conversion can carry: an inexact tick multiplies a
 * span by the tick rate before dividing by the counter rate, and that product
 * has to stay inside 64 bits. Only a counter wider than 32 bits can reach it,
 * and only long past any real span: 2.6 hours at 2 GHz with a 1000 Hz tick, 24
 * years at 24 MHz.
 */
#define TIMER_CORE_MAX_CONVERTIBLE_CYCLES ((UINT64_MAX / CONFIG_SYS_CLOCK_TICKS_PER_SEC) - 1U)

/* ... and never further than that. */
#define TIMER_CORE_COUNTER_SAFE_SPAN                                                               \
	MIN((uint64_t)TIMER_CORE_COUNTER_WIDTH_SPAN, TIMER_CORE_MAX_CONVERTIBLE_CYCLES)
#endif

/*
 * Furthest ahead of the last announce that unannounced time may run: what the
 * counter can still resolve. Nothing else bounds it, the alarm's reach being a
 * bound on one arm rather than on the total.
 */
#define TIMER_CORE_MAX_UNANNOUNCED_CYCLES TIMER_CORE_COUNTER_SAFE_SPAN

/*
 * Furthest one arm reaches: what the alarm can express, never past what the
 * counter can resolve. A deadline beyond this is walked to over several arms,
 * each announcing nothing until the last.
 */
#define TIMER_CORE_MAX_ARM_CYCLES                                                                  \
	((timer_core_cycles_t)MIN((uint64_t)TIMER_CORE_COUNTER_SAFE_SPAN,                          \
				  (uint64_t)TIMER_CORE_ALARM_MAX_CYCLES))

/*
 * Announce baseline, private to this translation unit.
 *
 * last_cycle is the cycle count of the most recent announce, held on the cycle
 * that tick last_tick starts on. Its low TIMER_CORE_COUNTER_WIDTH bits track the
 * hardware counter, so masking a delta against it is wrap-correct even for a
 * counter narrower than 64 bits. last_elapsed is the tick count most recently
 * reported to the kernel via sys_clock_elapsed().
 *
 * last_rem is what rounding that cycle up cost, in cycles scaled by the tick
 * rate:
 *
 *	last_rem = last_cycle * TICKS_PER_SEC - last_tick * CYCLES_PER_SEC
 *
 * so it is below the tick rate, and zero whenever a tick is a whole number of
 * cycles. Carrying it is what keeps the two conversion directions in step.
 */
static uint64_t timer_core_last_cycle;
static uint64_t timer_core_last_tick;
#if !TIMER_CORE_TICK_IS_WHOLE
static uint32_t timer_core_last_rem;
#endif

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
/* Truncated on purpose: this only chooses a tick type, and a rate that does not
 * divide the tick rate makes the real count smaller, so erring high is safe.
 */
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

#if TIMER_CORE_TICK_IS_WHOLE

/* Cycle position of tick @p t, counted from tick zero. */
static inline uint64_t timer_core_cyc_at_tick(uint64_t t)
{
	return t * TIMER_CORE_CYC_PER_TICK;
}

/* Whole ticks in @p c cycles counted from tick zero. */
static inline timer_core_ticks_t timer_core_ticks_at_cyc(uint64_t c)
{
	return (timer_core_ticks_t)(c / TIMER_CORE_CYC_PER_TICK);
}

/* Cycles from the announce baseline to the tick @p span ticks past it. */
static inline timer_core_cycles_t timer_core_span_cycles(timer_core_ticks_t span)
{
	return (timer_core_cycles_t)span * TIMER_CORE_CYC_PER_TICK;
}

/* Whole ticks spanned by @p c cycles measured from the announce baseline. */
static inline timer_core_ticks_t timer_core_ticks_in(timer_core_cycles_t c)
{
	return (timer_core_ticks_t)(c / TIMER_CORE_CYC_PER_TICK);
}

/* The same for a span the counter's width cannot express, which only the
 * recovery announce deals in.
 */
static inline uint64_t timer_core_ticks_in64(uint64_t c)
{
	return c / TIMER_CORE_CYC_PER_TICK;
}

/* Move the announce baseline on by @p dticks, keeping it on a tick position. */
static inline void timer_core_advance_baseline64(uint64_t dticks)
{
	timer_core_last_cycle += dticks * TIMER_CORE_CYC_PER_TICK;
	timer_core_last_tick += dticks;
}

static inline void timer_core_advance_baseline(timer_core_ticks_t dticks)
{
	timer_core_last_cycle += (timer_core_cycles_t)dticks * TIMER_CORE_CYC_PER_TICK;
	timer_core_last_tick += dticks;
}

#else /* a tick is not a whole number of cycles */

/*
 * The two directions, both exact, and neither dividing by the counter rate more
 * than once.
 *
 * Tick n starts at n * CYCLES_PER_SEC / TICKS_PER_SEC cycles, rounded up: the
 * tick has elapsed once the counter reaches that position, and where it is not
 * a whole cycle the counter has to reach the next one. Rounding down instead
 * arms every deadline a cycle short of the tick it aims at.
 *
 * Away from tick zero the position is carried by the baseline, which divides by
 * the Kconfig tick rate rather than by the counter rate.
 */

/* Cycle position of tick @p t, counted from tick zero. Split at the tick rate
 * so t * TIMER_CORE_CYCLES_PER_SEC is never formed.
 */
static inline uint64_t timer_core_cyc_at_tick(uint64_t t)
{
	uint64_t whole = t / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	uint64_t part = t % CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	return (whole * TIMER_CORE_CYCLES_PER_SEC) +
	       DIV_ROUND_UP(part * TIMER_CORE_CYCLES_PER_SEC, CONFIG_SYS_CLOCK_TICKS_PER_SEC);
}

/* What rounding tick @p t up to a whole cycle cost, in cycles scaled by the
 * tick rate. Whole seconds land on a whole cycle, so only the part of the tick
 * count below the tick rate contributes.
 */
static inline uint32_t timer_core_rem_at_tick(uint64_t t)
{
	uint64_t part = ((t % CONFIG_SYS_CLOCK_TICKS_PER_SEC) * TIMER_CORE_CYCLES_PER_SEC) +
			(CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U);

	return (CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) -
	       (uint32_t)(part % CONFIG_SYS_CLOCK_TICKS_PER_SEC);
}

/* Whole ticks in @p c cycles counted from tick zero. */
static inline timer_core_ticks_t timer_core_ticks_at_cyc(uint64_t c)
{
	return (timer_core_ticks_t)(c * CONFIG_SYS_CLOCK_TICKS_PER_SEC / TIMER_CORE_CYCLES_PER_SEC);
}

/*
 * Cycles from the announce baseline to the tick @p span ticks past it:
 * ceil((span * CYCLES_PER_SEC - last_rem) / TICKS_PER_SEC), with the rounding
 * term folded in so the numerator cannot go negative at span zero. Whole ticks
 * spanned by @p c cycles is the same relation inverted.
 *
 * Both divide, and on a 64-bit target that is what the hardware does anyway. A
 * 32-bit target has no 64-bit divide, so the same code there is a call into
 * libgcc, taken from the scheduler on whatever thread arms a timeout and paying
 * a stack frame for it. That target gets the split forms further down instead.
 */
#if defined(CONFIG_64BIT)

static inline timer_core_cycles_t timer_core_span_cycles(timer_core_ticks_t span)
{
	uint64_t n = ((uint64_t)span * TIMER_CORE_CYCLES_PER_SEC) +
		     ((CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) - timer_core_last_rem);

	return (timer_core_cycles_t)(n / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
}

static inline timer_core_ticks_t timer_core_ticks_in(timer_core_cycles_t c)
{
	c = (timer_core_cycles_t)MIN((uint64_t)c, TIMER_CORE_MAX_CONVERTIBLE_CYCLES);

	return (timer_core_ticks_t)((((uint64_t)c * CONFIG_SYS_CLOCK_TICKS_PER_SEC) +
				     timer_core_last_rem) /
				    TIMER_CORE_CYCLES_PER_SEC);
}

/* Move the announce baseline on by @p dticks, keeping it on a tick position and
 * its remainder with it.
 */
static inline void timer_core_advance_baseline(timer_core_ticks_t dticks)
{
	uint64_t n = ((uint64_t)dticks * TIMER_CORE_CYCLES_PER_SEC) +
		     ((CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) - timer_core_last_rem);

	timer_core_last_cycle += n / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	timer_core_last_rem = (CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) -
			      (uint32_t)(n % CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	timer_core_last_tick += dticks;
}

#else /* no 64-bit divide */

/*
 * Scaled numerator shared by the two tick-to-cycle helpers: the fractional
 * cycles owed for @p span ticks, plus what the baseline already holds, in units
 * of one cycle divided by the tick rate. Both terms are below the tick rate
 * times the span, so this stays inside 32 bits once the span is capped at
 * TIMER_CORE_MAX_FRAC_SPAN_TICKS, and the divisor is the Kconfig tick rate,
 * which the compiler turns into a multiply.
 */
static inline uint32_t timer_core_frac_num(timer_core_ticks_t span)
{
	return ((uint32_t)span * TIMER_CORE_CYC_REM) +
	       ((CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) - timer_core_last_rem);
}

/* Only the fraction goes through a division. */
static inline timer_core_cycles_t timer_core_span_cycles(timer_core_ticks_t span)
{
	return ((timer_core_cycles_t)span * TIMER_CORE_CYC_PER_TICK) +
	       (timer_core_frac_num(span) / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
}

/*
 * Multiply by a reciprocal of the counter rate rather than divide by it. The
 * reciprocal is truncated and the baseline remainder is left out, so the
 * estimate is never high and is low by at most a tick or two; walking it up
 * against the exact position settles it.
 */
static inline timer_core_ticks_t timer_core_ticks_in(timer_core_cycles_t c)
{
	timer_core_ticks_t n;

	c = (timer_core_cycles_t)MIN((uint64_t)c, (uint64_t)TIMER_CORE_MAX_UNANNOUNCED_CYCLES);
	n = (timer_core_ticks_t)(((uint64_t)(uint32_t)c * TIMER_CORE_CYC_RECIP) >> 32);
	n = MIN(n, (timer_core_ticks_t)TIMER_CORE_MAX_FRAC_SPAN_TICKS);

	while ((n < TIMER_CORE_MAX_FRAC_SPAN_TICKS) && (timer_core_span_cycles(n + 1U) <= c)) {
		n++;
	}

	return n;
}

/* Move the announce baseline on by @p dticks, keeping it on a tick position and
 * its remainder with it.
 */
static inline void timer_core_advance_baseline(timer_core_ticks_t dticks)
{
	uint32_t n = timer_core_frac_num(dticks);

	timer_core_last_cycle +=
		((uint64_t)dticks * TIMER_CORE_CYC_PER_TICK) + (n / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	timer_core_last_rem =
		(CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) - (n % CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	timer_core_last_tick += dticks;
}

#endif /* CONFIG_64BIT */

/*
 * The same two directions for a span the counter's width cannot express, which
 * only the recovery announce deals in. It is not on the arm path, so it pays
 * the wide division rather than carrying a second reciprocal for the case.
 */
static inline uint64_t timer_core_ticks_in64(uint64_t c)
{
	c = MIN(c, TIMER_CORE_MAX_CONVERTIBLE_CYCLES);

	return ((c * CONFIG_SYS_CLOCK_TICKS_PER_SEC) + timer_core_last_rem) /
	       TIMER_CORE_CYCLES_PER_SEC;
}

static inline void timer_core_advance_baseline64(uint64_t dticks)
{
	uint64_t n = (dticks * TIMER_CORE_CYCLES_PER_SEC) +
		     ((CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) - timer_core_last_rem);

	timer_core_last_cycle += n / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	timer_core_last_rem = (CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1U) -
			      (uint32_t)(n % CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	timer_core_last_tick += dticks;
}

#endif /* TIMER_CORE_TICK_IS_WHOLE */

/*
 * The arm span ceiling, in ticks: TIMER_CORE_MAX_UNANNOUNCED_CYCLES expressed in
 * the tick domain so the arm path can clamp there and never form a product
 * wider than the counter.
 *
 * It has to floor: a tick count rounded up would let the arm reach past what
 * the counter resolves. It is taken from tick zero rather than from the
 * baseline so it does not move with the baseline's rounding remainder, which
 * can only make it shorter by a tick than the span the baseline would allow.
 *
 * The division folds where both terms are build constants, which is the common
 * case. Where the rate is only known at run time it would be a real division on
 * every arm, so precompute it once alongside the cycles per tick instead.
 *
 * Never past what the fractional arithmetic carries either, since an inexact
 * tick multiplies the span by a remainder below the tick rate and keeps that
 * inside 32 bits. Only a counter both wide and slow reaches that, and the ticks
 * it gives up are ones the arm walks to in more than one step anyway.
 */
#if TIMER_CORE_TICK_IS_WHOLE || defined(CONFIG_64BIT)
#define TIMER_CORE_SPAN_TICKS_OF(c) timer_core_ticks_at_cyc(c)
#else
#define TIMER_CORE_SPAN_TICKS_OF(c)                                                                \
	MIN(timer_core_ticks_at_cyc(c), (timer_core_ticks_t)TIMER_CORE_MAX_FRAC_SPAN_TICKS)
#endif

#if defined(TIMER_CORE_PRECOMPUTE_CYC_PER_TICK)
static timer_core_ticks_t timer_core_max_span_ticks;
#define TIMER_CORE_MAX_SPAN_TICKS timer_core_max_span_ticks
#else
#define TIMER_CORE_MAX_SPAN_TICKS TIMER_CORE_SPAN_TICKS_OF(TIMER_CORE_MAX_UNANNOUNCED_CYCLES)
/* A tick wider than the counter can resolve leaves the masked delta ambiguous,
 * which no amount of re-arming recovers, so catch it here rather than at run
 * time. This needs the rate to be a build constant, so the cases where it is
 * not are checked in timer_core_init() instead.
 */
BUILD_ASSERT(TIMER_CORE_COUNTER_SAFE_SPAN >= TIMER_CORE_CYC_PER_TICK,
	     "a tick is longer than the counter can span: raise "
	     "CONFIG_SYS_CLOCK_TICKS_PER_SEC, or slow the counter");
#if !defined(TIMER_CORE_BACKEND_RELOAD)
BUILD_ASSERT(TIMER_CORE_MAX_ARM_CYCLES >= TIMER_CORE_CYC_PER_TICK,
	     "a tick is longer than the compare alarm reaches: raise "
	     "CONFIG_SYS_CLOCK_TICKS_PER_SEC, or slow the counter");
#endif
#endif

#if defined(TIMER_CORE_BACKEND_RELOAD)
/* A catch-up reload (floored at TIMER_CORE_ALARM_MIN_CYCLES because the deadline is
 * already due, or because the un-announced span is about to overrun
 * TIMER_CORE_MAX_UNANNOUNCED_CYCLES) is in flight. Programming a reload restarts the
 * counter, so a stream of set_timeout() calls arriving faster than the floor
 * would rewrite the reload before it can expire and postpone the announce
 * indefinitely; while this is set, timer_core_arm() leaves the hardware alone
 * so the pending fire gets through. Cleared by the announce.
 */
static bool timer_core_catchup;

/* Tick-aligned deadline currently armed in the hardware, or UINT64_MAX when
 * nothing is known to be armed (initially, and after each announce, which
 * consumes the programmed deadline). timer_core_arm() reprograms only when the
 * deadline actually moves. The kernel re-evaluates its earliest timeout on
 * every add and abort, a timeslice reset being one of each, so back-to-back
 * calls usually land on the same tick boundary. Rewriting a reload for those
 * is not merely churn: it restarts the counter, so a sustained stream of
 * rewrites keeps any period from ever completing and postpones the announce
 * for the duration of the stream. A comparator needs none of this, since
 * writing the same absolute value again changes nothing.
 */
static uint64_t timer_core_armed_deadline = UINT64_MAX;
#endif

/* Arm the comparator at an absolute cycle count, whatever the flavour of the
 * hardware match, so the callers below never branch on it.
 */
#if defined(TIMER_CORE_BACKEND_COMPARE_ORDERED)
static inline void timer_core_set_compare(uint64_t target)
{
	/* Ordered match: a target already behind the counter fires at once. */
	timer_driver_set_compare(target);
}
#elif defined(TIMER_CORE_BACKEND_COMPARE_EXACT)
/* Cycles a compare must be ahead of the counter for the match to be caught.
 * One by default: a comparator written while the counter is still below it
 * fires. Hardware that has to carry the write into the counter's clock domain
 * first misses a match that close, and states the lead that costs it.
 */
#ifndef TIMER_CORE_ALARM_LEAD_CYCLES
#define TIMER_CORE_ALARM_LEAD_CYCLES 1
#endif

/* True when @p target is no longer far enough ahead of @p now to be caught,
 * evaluated in the counter's own width so a narrow counter wraps correctly.
 */
static inline bool timer_core_target_passed(uint64_t target, uint64_t now)
{
	timer_core_cycles_t ahead = ((timer_core_cycles_t)target - (timer_core_cycles_t)now) &
				    (timer_core_cycles_t)TIMER_CORE_COUNTER_MASK;

	/* "Is the target still ahead" is the signed comparison, so the split is
	 * at half the counter width and nothing else: a property of the counter,
	 * not of the arming range or of the announce cadence. The core never arms
	 * past TIMER_CORE_COUNTER_SAFE_SPAN, which is at most this, so a legitimate
	 * future target is never mistaken for a wrapped one.
	 */
	return (ahead < TIMER_CORE_ALARM_LEAD_CYCLES) ||
	       (ahead > (timer_core_cycles_t)(TIMER_CORE_COUNTER_MASK >> 1));
}

/* Arm an equality-match comparator so the interrupt cannot be lost.
 *
 * Such hardware fires only while the counter equals the comparator, so a value
 * written after the counter has gone past it is missed until the counter comes
 * all the way round. Write it, then read the counter back: if the target is no
 * longer far enough ahead, push it out by TIMER_CORE_ALARM_LEAD_CYCLES and try
 * again, doubling the push so a counter that keeps overtaking us still
 * terminates. The bump costs accuracy only in the case where the deadline was
 * already unmeetable.
 *
 * This is what lets an exact-match driver drop the minimum-delay floor it would
 * otherwise need: the floor exists to keep the requested deadline far enough
 * ahead to be caught, and the loop below establishes that directly, at the cost
 * of one extra counter read on the deadlines that were already close.
 */
static inline void timer_core_set_compare(uint64_t target)
{
	timer_driver_set_compare(target);

	timer_core_cycles_t now = timer_driver_cycle_get();

	if (unlikely(timer_core_target_passed(target, now))) {
		uint32_t bump = TIMER_CORE_ALARM_LEAD_CYCLES;

		do {
			target = now + bump;
			bump *= 2;
			timer_driver_set_compare(target);
			now = timer_driver_cycle_get();
		} while (timer_core_target_passed(target, now));
	}
}
#endif

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

	return timer_core_ticks_in(delta);
}

/* Program the timer for a tick-aligned deadline `ticks` out from the last
 * reported elapsed. Called with the system clock lock held.
 */
static void timer_core_arm(uint32_t ticks)
{
#if defined(TIMER_CORE_BACKEND_RELOAD)
	uint64_t deadline_tick = timer_core_last_tick + timer_core_last_elapsed + ticks;

	if (deadline_tick == timer_core_armed_deadline) {
		return;
	}
	timer_core_armed_deadline = deadline_tick;

	/*
	 * Relative reload: the cycles from the last announce to the tick-aligned
	 * deadline, minus what has already elapsed since then. Both terms are in
	 * the counter's cycle domain (the elapsed part masked to TIMER_CORE_COUNTER_WIDTH),
	 * so this stays correct across a counter wrap.
	 *
	 * The span is clamped in the tick domain, before the multiply, so the
	 * product stays inside that width.
	 *
	 * Capping it is what keeps a stream of set_timeout() calls with no
	 * intervening announce from pushing the fire point out indefinitely:
	 * re-arming restarts the counter, `done` would grow past
	 * TIMER_CORE_COUNTER_MASK and the masked delta would wrap, silently losing
	 * the elapsed span. Once `done` reaches the span the subtraction below
	 * saturates and the ALARM_MIN_CYCLES floor forces a near-immediate fire,
	 * hence an announce that resets the baseline. The COMPARE path gets the same
	 * invariant from clamping its absolute deadline.
	 */
	timer_core_ticks_t span = TIMER_CORE_MAX_SPAN_TICKS;

	if ((ticks <= span) && (timer_core_last_elapsed <= (span - ticks))) {
		span = timer_core_last_elapsed + ticks;
	}

	timer_core_cycles_t want = timer_core_span_cycles(span);
	timer_core_cycles_t done = timer_core_cycles_since(timer_core_last_cycle);

	/*
	 * Saturate rather than go signed: only the sign of want - done matters, and
	 * its magnitude when positive. A starved ISR, elapsed past the deadline,
	 * lands on the floor below whether the difference is zero or hugely
	 * negative, so zero stands in for every negative value.
	 */
	timer_core_cycles_t rel = (want > done) ? (want - done) : 0;

	/* Only where the alarm binds before the counter does; the span clamp
	 * above already holds `rel` inside the counter's reach, so this folds
	 * away for hardware whose alarm covers the whole span.
	 */
	if ((TIMER_CORE_MAX_ARM_CYCLES < TIMER_CORE_MAX_UNANNOUNCED_CYCLES) &&
	    (rel > TIMER_CORE_MAX_ARM_CYCLES)) {
		rel = TIMER_CORE_MAX_ARM_CYCLES;
	}

	if (rel < TIMER_CORE_ALARM_MIN_CYCLES) {
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
		rel = TIMER_CORE_ALARM_MIN_CYCLES;
	}
	timer_driver_set_reload(rel);
#else /* compare backends */
	/*
	 * Absolute, tick-aligned deadline, reached as the announce baseline plus a
	 * relative span. last_cycle is the cycle count at last_tick exactly,
	 * which timer_core_init() establishes and the announce and the rescale
	 * maintain, so the deadline is
	 *
	 *   (last_tick + last_elapsed + ticks) * CYC = last_cycle + (last_elapsed + ticks) * CYC
	 *
	 * and the clamp, which subtracts last_cycle straight back off, is on that
	 * span alone. Forming it from the baseline keeps the arithmetic in the
	 * counter's own width instead of the baseline's: on a 32-bit target with a
	 * 32-bit counter the multiply, the compare and the clamp are all
	 * single-register work. It is also what an inexact tick needs at any
	 * width, the baseline carrying the rounding the absolute position cannot.
	 *
	 * The span is clamped in the tick domain, before the multiply, so the
	 * product cannot overflow that width. The clamp is never the tighter of
	 * the two: it is TIMER_CORE_MAX_UNANNOUNCED_CYCLES in ticks, and the
	 * alarm's reach below is at most that.
	 */
	timer_core_ticks_t span = TIMER_CORE_MAX_SPAN_TICKS;

	if ((ticks <= span) && (timer_core_last_elapsed <= (span - ticks))) {
		span = timer_core_last_elapsed + ticks;
	}
	timer_core_cycles_t offset = timer_core_span_cycles(span);

	if ((TIMER_CORE_MAX_ARM_CYCLES < TIMER_CORE_MAX_UNANNOUNCED_CYCLES) &&
	    (offset > TIMER_CORE_MAX_ARM_CYCLES)) {
		offset = TIMER_CORE_MAX_ARM_CYCLES;
	}
	timer_core_set_compare(timer_core_last_cycle + offset);
#endif
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

	timer_core_advance_baseline(dticks);
	timer_core_last_elapsed = 0;
#if defined(TIMER_CORE_BACKEND_RELOAD)
	/* The programmed deadline is consumed (or obsolete): the kernel decides
	 * the next one after the announce, and it must reach the hardware even
	 * if it lands on the same tick.
	 */
	timer_core_armed_deadline = UINT64_MAX;
	timer_core_catchup = false;
#endif

#if !defined(TIMER_CORE_BACKEND_RELOAD)
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Re-arm the comparator one tick out. A RELOAD counter reloads
		 * itself from the LOAD set at init, so it needs nothing here.
		 */
		timer_core_arm(1);
	}
#endif

	/* The baseline above moved by the whole delta, so it stays aligned with
	 * the counter. Only what the kernel is told is clamped to the width of
	 * its interface, and that difference is uptime drift, which is what
	 * sloppy idle allows in the first place.
	 */
	sys_clock_announce_locked(timer_core_ticks_clamp(dticks), key);
}

/* Announce a span of cycles the driver worked out for itself, rather than one
 * read from the counter, with the clock lock already held. @p key is the key
 * returned by the driver's sys_clock_lock() and is consumed here.
 *
 * For a driver recovering elapsed time from somewhere other than its counter,
 * a low-power companion that kept time while the counter was stopped, where the
 * span can outrun what the counter's width can express.
 *
 * The arithmetic here is 64-bit whatever the counter's width, since that span is
 * exactly the case the width cannot cover. That is why this is separate from
 * timer_core_announce_from(), which stays in the counter's own width: only the
 * recovery path pays for the wider math.
 *
 * Only whole ticks are announced. The sub-tick remainder is left where the
 * driver's own counter still holds it, for the next delta to pick up.
 *
 * inline so the drivers with no such recovery path do not trip
 * -Wunused-function.
 */
static inline void timer_core_announce_cycles64_from(k_spinlock_key_t key, uint64_t cycles)
{
	uint64_t dticks = timer_core_ticks_in64(cycles);

	timer_core_advance_baseline64(dticks);
	timer_core_last_elapsed = 0;
#if defined(TIMER_CORE_BACKEND_RELOAD)
	timer_core_armed_deadline = UINT64_MAX;
	timer_core_catchup = false;
#endif

	/*
	 * Not timer_core_ticks_clamp(): its argument is timer_core_ticks_t, the
	 * very width this span is allowed to outrun, so the value would be
	 * truncated on the way in rather than clamped. Clamp against the
	 * announce interface itself.
	 */
	sys_clock_announce_locked((uint32_t)MIN(dticks, (uint64_t)UINT32_MAX), key);
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
 * Full-width cycle count, in the counter's own domain: the announce baseline
 * (which carries the extended upper bits) plus the masked forward delta. A
 * driver scaling the count into another unit builds its own getters on this.
 *
 * The read is taken under the clock lock when it cannot be trusted atomically:
 *
 *   - timer_driver_cycle_get() is a synthesized count, not a single register read
 *     (TIMER_CORE_COUNTER_NONATOMIC): the lock serialises it against the ISR and
 *     reprogram paths that update the same software state; or
 *   - the full 64-bit baseline is read on a narrower counter (the getter below
 *     needs all 64 bits, which is not a single load on a 32-bit CPU).
 *
 * A narrow counter feeding only the 32-bit getter does NOT need this: that path
 * reconstructs from the baseline's low word, a single atomic load, so it stays
 * lock-free (see sys_clock_cycle_get_32()). Keeping the lock out of the 32-bit
 * getter matters because it is on the thread-usage timestamp hot path.
 */
static inline uint64_t timer_core_cycle_get(void)
{
#if defined(TIMER_CORE_COUNTER_NONATOMIC) || TIMER_CORE_COUNTER_WIDTH < 64
	k_spinlock_key_t key = sys_clock_lock();
	uint64_t ret = timer_core_last_cycle + timer_core_cycles_since(timer_core_last_cycle);

	sys_clock_unlock(key);
	return ret;
#else
	/* A counter as wide as the count itself needs no extending. */
	return timer_driver_cycle_get();
#endif
}

/*
 * A driver that states its own TIMER_CORE_CYCLES_PER_SEC has a counter running at
 * a rate the kernel does not know about, so what this core would emit, the raw
 * count, is not what the kernel expects to read. The 32-bit getter is then the
 * driver's, in whatever unit it settles on, and this core emits no 64-bit one
 * either: that would be in the counter's own domain and disagree. A driver
 * wanting a 64-bit getter in its unit supplies that too.
 */
#if defined(TIMER_CORE_DRIVER_CYCLES_PER_SEC)
#if !defined(TIMER_CORE_HAVE_CYCLE_GET_32)
#error "a driver setting TIMER_CORE_CYCLES_PER_SEC must define " \
	"TIMER_CORE_HAVE_CYCLE_GET_32 and supply sys_clock_cycle_get_32()"
#endif
#ifndef TIMER_CORE_HAVE_CYCLE_GET_64
#define TIMER_CORE_HAVE_CYCLE_GET_64
#endif
#endif

#if !defined(TIMER_CORE_HAVE_CYCLE_GET_32)
uint32_t sys_clock_cycle_get_32(void)
{
#if defined(TIMER_CORE_COUNTER_NONATOMIC)
	/* Synthesized read: serialise the get and baseline under the lock. */
	return (uint32_t)timer_core_cycle_get();
#elif TIMER_CORE_COUNTER_WIDTH < 32
	/* Narrow atomic counter: extend from the baseline's low word. Both reads
	 * are single loads, and pairing one baseline read with one counter read
	 * yields the true position regardless of an announce in between, so this
	 * needs no lock.
	 */
	uint32_t base = (uint32_t)timer_core_last_cycle;
	uint32_t delta = ((uint32_t)timer_driver_cycle_get() - base) &
			 (uint32_t)TIMER_CORE_COUNTER_MASK;

	return base + delta;
#else
	return (uint32_t)timer_driver_cycle_get();
#endif
}
#endif

#if !defined(TIMER_CORE_HAVE_CYCLE_GET_64)
uint64_t sys_clock_cycle_get_64(void)
{
	return timer_core_cycle_get();
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
	/* The rate just changed, so everything derived from it is stale: the
	 * cycles per tick, and the arm ceiling, which is a cycle span expressed
	 * in ticks and so says more than the counter can deliver once the rate
	 * goes up. Re-deriving the ceiling reads the rate through
	 * TIMER_CORE_CYCLES_PER_SEC, which the caller has already updated, as
	 * the baseline below does.
	 */
	timer_core_cyc_per_tick = to_hz / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#if !defined(CONFIG_64BIT)
	timer_core_cyc_rem = TIMER_CORE_CYC_REM_OF(to_hz);
	timer_core_cyc_recip = TIMER_CORE_CYC_RECIP_OF(to_hz);
#endif
	timer_core_max_span_ticks = TIMER_CORE_SPAN_TICKS_OF(TIMER_CORE_MAX_UNANNOUNCED_CYCLES);
#else
	ARG_UNUSED(to_hz);
#endif
	/*
	 * Re-express the announce baseline in the new cycle domain. Deriving it
	 * from last_tick (a frequency-independent tick count) keeps it an exact
	 * tick position, the invariant the arm and announce paths rely
	 * on. Scaling the old cycle value directly would leave it a fraction of a
	 * tick off and, on the COMPARE arm, could make (deadline - last_cycle)
	 * underflow. The caller has already rescaled its own cycle counter.
	 */
	timer_core_last_cycle = timer_core_cyc_at_tick(timer_core_last_tick);
#if !TIMER_CORE_TICK_IS_WHOLE
	timer_core_last_rem = timer_core_rem_at_tick(timer_core_last_tick);
#endif
}

/* Prime the calling CPU's timer one tick ahead of the shared baseline. An SMP
 * driver calls this from smp_timer_init() so it need not touch the core's
 * private baseline.
 */
static inline void timer_core_smp_prime(void)
{
#if defined(TIMER_CORE_BACKEND_RELOAD)
	timer_driver_set_reload(timer_core_span_cycles(1));
#else
	timer_core_set_compare(timer_core_last_cycle + timer_core_span_cycles(1));
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
#if !defined(CONFIG_64BIT)
	timer_core_cyc_rem = TIMER_CORE_CYC_REM_OF(TIMER_CORE_CYCLES_PER_SEC);
	timer_core_cyc_recip = TIMER_CORE_CYC_RECIP_OF(TIMER_CORE_CYCLES_PER_SEC);
#endif
	timer_core_max_span_ticks = TIMER_CORE_SPAN_TICKS_OF(TIMER_CORE_MAX_UNANNOUNCED_CYCLES);
	/* The rate not being a constant expression, the non-zero check the
	 * constant case gets at build time happens here instead.
	 */
	__ASSERT(TIMER_CORE_CYC_PER_TICK != 0, "timer counter rate is below the tick rate");
	/* Both sides are widened so the comparison is not typed against the
	 * runtime rate's uint32_t, which a 64-bit counter's span cannot reach
	 * and which the compiler would then flag as always true.
	 */
	__ASSERT((uint64_t)TIMER_CORE_COUNTER_SAFE_SPAN >= (uint64_t)TIMER_CORE_CYC_PER_TICK,
		 "a tick is longer than the counter can span");
#if !defined(TIMER_CORE_BACKEND_RELOAD)
	__ASSERT(TIMER_CORE_MAX_ARM_CYCLES >= TIMER_CORE_CYC_PER_TICK,
		 "a tick is longer than the compare alarm reaches");
#endif
#endif
	/* Seed the baseline from the counter. The baseline is still zero here, so
	 * the conversion is the one from tick zero, and the counter read being
	 * inside the counter's width, the tick count it divides down to and the
	 * cycle count that multiplies back up both are too.
	 */
#if TIMER_CORE_TICK_IS_WHOLE
	timer_core_last_tick = timer_core_ticks_in(timer_driver_cycle_get());
	timer_core_last_cycle = timer_core_cyc_at_tick(timer_core_last_tick);
#else
	/* Walked out from zero rather than assigned, because the remainder that
	 * goes with the cycle position is what the walk maintains.
	 */
	timer_core_advance_baseline(timer_core_ticks_in(timer_driver_cycle_get()));
#endif
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

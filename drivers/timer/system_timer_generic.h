/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic tickless system timer core
 *
 * Common code for tickless system timer drivers. The driver supplies a few
 * hardware primitives (read the counter, arm the alarm) and this file does
 * the rest: converting cycles to ticks, tracking what was last announced,
 * computing deadlines and clamping them to what the hardware can reach.
 *
 * This header holds the actual sys_clock_set_timeout(), sys_clock_elapsed()
 * and sys_clock_cycle_get_32/64() for the kernel, so include it once, from
 * the driver's .c file, after the definitions listed below.
 *
 * The driver must define one backend:
 *
 *   TIMER_CORE_BACKEND_COMPARE_ORDERED
 *	Free-running counter with a compare register that fires once the
 *	counter reaches or passes it. A value already in the past fires
 *	right away. Examples: arm_arch, riscv_machine.
 *
 *   TIMER_CORE_BACKEND_COMPARE_EXACT
 *	Same, but the compare fires only on an exact match. A value written
 *	too late is missed until the counter wraps, so the core verifies
 *	each write (see timer_core_set_compare()). Examples: hpet, mips_cp0.
 *
 *   TIMER_CORE_BACKEND_RELOAD
 *	A timer that fires after a given delay and then reloads by itself,
 *	like a down-counter. On a ticking (not tickless) kernel, the driver
 *	sets it to one tick at init and the core leaves it alone.
 *
 * If the alarm only works with its interrupt enabled, enable it in the arming
 * function. The core doesn't track interrupt masking.
 *
 * The driver must also provide:
 *
 *   static uint32_t/uint64_t timer_driver_cycle_get(void)
 *	Return the raw counter value, wraps and all. Give its width in
 *	TIMER_CORE_COUNTER_WIDTH and the core handles the wraps. uint32_t for
 *	up to 32 bits, uint64_t otherwise.
 *
 *   static void timer_driver_set_compare(uint32_t/uint64_t cycles)   (COMPARE)
 *	Just write @p cycles to the compare register. Don't check whether it
 *	is already in the past: depending on the backend, the hardware or the
 *	core deals with that. A 32-bit compare register can take uint32_t.
 *
 *   static void timer_driver_set_reload(uint32_t/uint64_t cycles)   (RELOAD)
 *	Fire the interrupt @p cycles from now. The core keeps @p cycles
 *	between TIMER_CORE_ALARM_MIN_CYCLES and TIMER_CORE_ALARM_MAX_CYCLES.
 *	uint32_t if TIMER_CORE_ALARM_MAX_CYCLES fits in 32 bits, uint64_t
 *	otherwise.
 *
 * Optional settings:
 *
 *   TIMER_CORE_CYCLES_PER_SEC
 *	Counter rate in Hz. Defaults to the kernel's hardware clock rate. Set
 *	it if the counter runs at another rate, e.g. prescaled. The core
 *	derives TIMER_CORE_CYC_PER_TICK from it; a driver can read that one
 *	but must not define it.
 *
 *   TIMER_CORE_CYCLES_PER_SEC_RUNTIME
 *	Define it when TIMER_CORE_CYCLES_PER_SEC is a variable, e.g. a rate
 *	read from the clock controller. Give that variable its value before
 *	calling timer_core_init().
 *
 *   TIMER_CORE_COUNTER_WIDTH
 *	Counter width in bits, up to 64. Defaults to the CPU register width.
 *	Set it for any other width, including a 64-bit counter on a 32-bit
 *	CPU.
 *
 *   TIMER_CORE_COUNTER_NONMONOTONIC
 *	The counter can briefly read lower than a value already seen (e.g. a
 *	global timer under QEMU SMP). The core then treats a backwards read
 *	as no time passed rather than a huge jump.
 *
 *   TIMER_CORE_COUNTER_NONATOMIC
 *	timer_driver_cycle_get() isn't a single register read but a count
 *	built in software, from state the ISR also updates. The core then
 *	always reads it under the clock lock.
 *
 *   TIMER_CORE_HAVE_CYCLE_GET_32, TIMER_CORE_HAVE_CYCLE_GET_64
 *	Define one or both if the sys_clock_cycle_get_32/64() provided here
 *	don't fit, and the driver provides its own instead, e.g. to convert
 *	the counter to the kernel's rate.
 *
 *   TIMER_CORE_ALARM_MAX_CYCLES
 *	Largest value the arming function accepts. Defaults to the full
 *	counter range. Set it when the compare or reload register is
 *	narrower than the counter, or is a separate device. State the
 *	hardware limit only; the core adds its own margin.
 *
 *   TIMER_CORE_ALARM_MIN_CYCLES   (RELOAD)
 *	Smallest reload, in cycles. Defaults to 1.
 *
 *   TIMER_CORE_ALARM_LEAD_CYCLES   (COMPARE_EXACT)
 *	How far ahead of the counter a compare value must be to be caught.
 *	Defaults to 1. Raise it if the hardware needs time to sync the
 *	write to the counter's clock.
 *
 * The rest of the driver is:
 *   - the ISR: acknowledge the hardware, then call timer_core_announce();
 *   - init: connect the IRQ, then call timer_core_init();
 *   - on SMP, smp_timer_init(): call timer_core_smp_prime().
 */

#ifndef ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_
#define ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_

#include <zephyr/drivers/timer/system_timer_lpm.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#if (defined(TIMER_CORE_BACKEND_COMPARE_ORDERED) + \
	defined(TIMER_CORE_BACKEND_COMPARE_EXACT) + \
	defined(TIMER_CORE_BACKEND_RELOAD)) != 1
#error "define exactly one backend: TIMER_CORE_BACKEND_COMPARE_ORDERED, " \
	"TIMER_CORE_BACKEND_COMPARE_EXACT or TIMER_CORE_BACKEND_RELOAD"
#endif

/* Counter rate. Remember whether the driver set it: the cycle getters further
 * down depend on that.
 */
#if defined(TIMER_CORE_CYCLES_PER_SEC)
#define TIMER_CORE_DRIVER_CYCLES_PER_SEC
#endif

#if !defined(TIMER_CORE_CYCLES_PER_SEC)
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#define TIMER_CORE_CYCLES_PER_SEC sys_clock_hw_cycles_per_sec()
#else
/* The Kconfig value directly: sys_clock_hw_cycles_per_sec() has a cast,
 * which #if can't evaluate.
 */
#define TIMER_CORE_CYCLES_PER_SEC CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define TIMER_CORE_CYC_PER_TICK_IS_CONSTANT
#endif
#endif

/*
 * Cycles per tick: a constant if the rate is, or computed once in
 * timer_core_init() otherwise.
 */
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME) || \
	defined(TIMER_CORE_CYCLES_PER_SEC_RUNTIME)
static uint32_t timer_core_cyc_per_tick;
#define TIMER_CORE_CYC_PER_TICK timer_core_cyc_per_tick
#define TIMER_CORE_PRECOMPUTE_CYC_PER_TICK
#else
#define TIMER_CORE_CYC_PER_TICK (TIMER_CORE_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
/*
 * This is zero if the counter runs slower than the tick rate, and we divide
 * by it. Catch that at build time if constant, or in timer_core_init()
 * otherwise.
 */
#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#define TIMER_CORE_CHECK_CYC_PER_TICK_AT_INIT
#else
BUILD_ASSERT(TIMER_CORE_CYC_PER_TICK != 0, "timer counter rate is below the tick rate");
#endif
#endif

#if !defined(TIMER_CORE_COUNTER_WIDTH)
#define TIMER_CORE_COUNTER_WIDTH (__SIZEOF_LONG__ * 8)
#endif

#define TIMER_CORE_COUNTER_MASK (UINT64_MAX >> (64 - TIMER_CORE_COUNTER_WIDTH))

/*
 * Type for a counter value, or the difference between two. 32 bits when the
 * counter fits, so a 32-bit CPU does the tick math in one register. The
 * baseline below is always 64 bits, as it keeps counting past the counter's
 * wrap.
 */
#if TIMER_CORE_COUNTER_WIDTH <= 32
typedef uint32_t timer_core_cycles_t;
#else
typedef uint64_t timer_core_cycles_t;
#endif

/* The alarm is usually part of the same counter. */
#ifndef TIMER_CORE_ALARM_MAX_CYCLES
#define TIMER_CORE_ALARM_MAX_CYCLES TIMER_CORE_COUNTER_MASK
#endif

#ifndef TIMER_CORE_ALARM_MIN_CYCLES
#define TIMER_CORE_ALARM_MIN_CYCLES 1
#endif

/*
 * How far past the last announce the counter can go and still be measured
 * correctly.
 *
 * Half the counter range, leaving the other half as slack in case the
 * interrupt comes late. With TIMER_CORE_COUNTER_NONMONOTONIC, the upper half
 * is kept to detect backwards reads, so this is a quarter, with the next
 * quarter as slack.
 */
#ifdef TIMER_CORE_COUNTER_NONMONOTONIC
#define TIMER_CORE_COUNTER_SAFE_SPAN (TIMER_CORE_COUNTER_MASK >> 2)
#else
#define TIMER_CORE_COUNTER_SAFE_SPAN (TIMER_CORE_COUNTER_MASK >> 1)
#endif

/*
 * Longest time allowed between two announces, in cycles. Only the counter
 * limits this: an alarm with a shorter reach just takes several arms.
 */
#define TIMER_CORE_MAX_UNANNOUNCED_CYCLES TIMER_CORE_COUNTER_SAFE_SPAN

/*
 * Furthest a single arm can reach: the alarm's limit, capped by the counter's.
 * A deadline further out takes several arms.
 */
#define TIMER_CORE_HW_MAX_ARM_CYCLES                                                               \
	MIN((uint64_t)TIMER_CORE_COUNTER_SAFE_SPAN, (uint64_t)TIMER_CORE_ALARM_MAX_CYCLES)

/*
 * QEMU icount without sleep skips idle time by jumping straight to the next
 * armed deadline. With nothing pending, that deadline is SYS_CLOCK_MAX_WAIT
 * ticks out (248 days at 100 Hz), and a few hundred such jumps overflow
 * QEMU's 64-bit nanosecond clock, after which no timer fires again. So cap
 * each arm at one second there. This is emulator policy, not a hardware
 * limit, so it isn't part of TIMER_CORE_ALARM_MAX_CYCLES.
 */
#if defined(CONFIG_QEMU_ICOUNT) && !defined(CONFIG_QEMU_ICOUNT_SLEEP)
#define TIMER_CORE_MAX_ARM_CYCLES                                                                  \
	((timer_core_cycles_t)MIN(TIMER_CORE_HW_MAX_ARM_CYCLES,                                    \
				  (uint64_t)TIMER_CORE_CYCLES_PER_SEC))
#else
#define TIMER_CORE_MAX_ARM_CYCLES ((timer_core_cycles_t)TIMER_CORE_HW_MAX_ARM_CYCLES)
#endif

/*
 * Where the last announce left us.
 *
 * last_cycle: counter value at the last announce, always on a tick boundary
 *             (last_tick * TIMER_CORE_CYC_PER_TICK). Its low bits follow the
 *             hardware counter, so differences against it survive a wrap.
 * last_tick:  ticks announced so far.
 * last_elapsed (below): the last tick count sys_clock_elapsed() returned.
 */
static uint64_t timer_core_last_cycle;
static uint64_t timer_core_last_tick;

/* Cycles from @p from to now, correct even if the counter wrapped in between. */
static inline timer_core_cycles_t timer_core_cycles_since(uint64_t from)
{
	return ((timer_core_cycles_t)timer_driver_cycle_get() - (timer_core_cycles_t)from) &
	       (timer_core_cycles_t)TIMER_CORE_COUNTER_MASK;
}

/* Ticks in the full counter range: an upper bound on any tick count between
 * two announces.
 *
 * The tick length matters as much as the counter width: a 48-bit counter at
 * 100000 cycles per tick tops out near 2.8e9 ticks, which fits in 32 bits.
 * Divide in #if when cycles per tick is constant, or go by the width alone
 * otherwise.
 */
#if defined(TIMER_CORE_CYC_PER_TICK_IS_CONSTANT)
#define TIMER_CORE_MAX_TICKS (TIMER_CORE_COUNTER_MASK / TIMER_CORE_CYC_PER_TICK)
#else
#define TIMER_CORE_MAX_TICKS TIMER_CORE_COUNTER_MASK
#endif

/* Type for tick counts inside the core. 64 bits when the count can exceed 32
 * bits: with no timeout pending, nothing announces and the count keeps
 * growing. The kernel interface is 32 bits, so timer_core_ticks_clamp() is
 * used where a count is passed to it.
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

/*
 * TIMER_CORE_MAX_UNANNOUNCED_CYCLES in ticks, for clamping before multiplying
 * so the cycle count can't overflow. A constant if the rate is, or computed
 * once in timer_core_init() otherwise.
 */
#if defined(TIMER_CORE_PRECOMPUTE_CYC_PER_TICK)
static timer_core_ticks_t timer_core_max_span_ticks;
#define TIMER_CORE_MAX_SPAN_TICKS timer_core_max_span_ticks
#else
#define TIMER_CORE_MAX_SPAN_TICKS (TIMER_CORE_MAX_UNANNOUNCED_CYCLES / TIMER_CORE_CYC_PER_TICK)
#if !defined(TIMER_CORE_CHECK_CYC_PER_TICK_AT_INIT)
/* A tick must fit in what the counter can measure. Checked here if constant,
 * or in timer_core_init() otherwise. Only the counter matters: if the alarm
 * can't cover a whole tick in one arm, the core just arms it several times.
 */
BUILD_ASSERT(TIMER_CORE_COUNTER_SAFE_SPAN >= TIMER_CORE_CYC_PER_TICK,
	     "a tick is longer than the counter can span: raise "
	     "CONFIG_SYS_CLOCK_TICKS_PER_SEC, or slow the counter");
#endif
#endif

#if defined(TIMER_CORE_BACKEND_RELOAD)
/* Set when timer_core_arm() writes a minimum reload because the deadline is
 * already due. Until the announce clears it, later calls that are also due
 * skip the write, so they don't keep restarting the counter.
 */
static bool timer_core_catchup;

/* Deadline in the hardware, in ticks, or UINT64_MAX if none. set_timeout()
 * often repeats the same deadline, since the kernel recomputes it on every
 * timeout add and abort. Rewriting a reload restarts the counter, so a
 * steady stream of repeats would keep the interrupt from ever firing. Skip
 * the write when the deadline hasn't changed.
 */
static uint64_t timer_core_armed_deadline = UINT64_MAX;
#endif

/* Set the compare register to an absolute counter value. Each backend
 * handles a value already in the past its own way.
 */
#if defined(TIMER_CORE_BACKEND_COMPARE_ORDERED)
static inline void timer_core_set_compare(uint64_t target)
{
	/* A value already passed fires right away. */
	timer_driver_set_compare(target);
}
#elif defined(TIMER_CORE_BACKEND_COMPARE_EXACT)
#ifndef TIMER_CORE_ALARM_LEAD_CYCLES
#define TIMER_CORE_ALARM_LEAD_CYCLES 1
#endif

/* True when @p target is no longer far enough ahead of @p now to be caught. */
static inline bool timer_core_target_passed(uint64_t target, uint64_t now)
{
	timer_core_cycles_t ahead = ((timer_core_cycles_t)target - (timer_core_cycles_t)now) &
				    (timer_core_cycles_t)TIMER_CORE_COUNTER_MASK;

	/* A target already behind the counter wraps to a large value here. The
	 * core never arms more than half the counter range ahead, so anything
	 * above half is a target already passed.
	 */
	return (ahead < TIMER_CORE_ALARM_LEAD_CYCLES) ||
	       (ahead > (timer_core_cycles_t)(TIMER_CORE_COUNTER_MASK >> 1));
}

/* Write an exact-match compare register without losing the interrupt.
 *
 * If the counter passes the value before it is written, the match only
 * happens after the counter wraps all the way round. So write, read the
 * counter back, and if the value is no longer ahead, move it further out and
 * try again. The step doubles each time, so this ends even if the counter
 * keeps catching up.
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

/* Whole ticks since the last announce. */
static inline timer_core_ticks_t timer_core_delta_ticks(void)
{
	timer_core_cycles_t delta = timer_core_cycles_since(timer_core_last_cycle);

#ifdef TIMER_CORE_COUNTER_NONMONOTONIC
	/*
	 * A counter read slightly behind the last announce gives a difference
	 * near the full counter range. The core never lets more than a quarter
	 * of the range go by (TIMER_CORE_COUNTER_SAFE_SPAN), so anything in the
	 * upper half is such a backwards read: count it as no time passed.
	 */
	if (delta > (timer_core_cycles_t)(TIMER_CORE_COUNTER_MASK >> 1)) {
		return 0;
	}
#endif

	return delta / TIMER_CORE_CYC_PER_TICK;
}

/* Arm the timer for `ticks` past the last elapsed count the kernel got, on a
 * tick boundary. Called with the clock lock held.
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
	 * The reload is the cycles from the last announce to the deadline,
	 * minus those already gone by.
	 *
	 * Clamp the span in ticks, before multiplying, so it can't overflow.
	 * The span counts from the last announce, so the interrupt fires
	 * within TIMER_CORE_MAX_UNANNOUNCED_CYCLES of it, however many
	 * set_timeout() calls may come in between. Without that, each call
	 * would re-arm from now, and a steady stream of them could hold off
	 * the interrupt for longer than the counter range. The cycle count
	 * since the last announce would then wrap, and that time would never
	 * be announced.
	 */
	timer_core_ticks_t span = TIMER_CORE_MAX_SPAN_TICKS;

	if ((ticks <= span) && (timer_core_last_elapsed <= (span - ticks))) {
		span = timer_core_last_elapsed + ticks;
	}

	timer_core_cycles_t want = (timer_core_cycles_t)span * TIMER_CORE_CYC_PER_TICK;
	timer_core_cycles_t done = timer_core_cycles_since(timer_core_last_cycle);

	/* Deadline already passed: 0, and the minimum reload below takes over. */
	timer_core_cycles_t rel = (want > done) ? (want - done) : 0;

	/* Only needed if the alarm reaches less far than the counter. Otherwise
	 * the span clamp above covers it, and this compiles away.
	 */
	if ((TIMER_CORE_MAX_ARM_CYCLES < TIMER_CORE_MAX_UNANNOUNCED_CYCLES) &&
	    (rel > TIMER_CORE_MAX_ARM_CYCLES)) {
		rel = TIMER_CORE_MAX_ARM_CYCLES;
	}

	if (rel < TIMER_CORE_ALARM_MIN_CYCLES) {
		/*
		 * Due now: fire as soon as possible. If a minimum reload is
		 * already pending, let it fire rather than restart the counter.
		 * Otherwise set_timeout() calls coming faster than the minimum
		 * reload (e.g. timeslice resets from a syscall-heavy thread)
		 * would push it back for as long as they keep coming, and time
		 * would stop. Nothing can be due sooner than the minimum anyway.
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
	 * The deadline is (last_tick + last_elapsed + ticks) ticks, in cycles.
	 * Since last_cycle == last_tick * CYC exactly (CYC being
	 * TIMER_CORE_CYC_PER_TICK), that is
	 *
	 *   last_cycle + (last_elapsed + ticks) * CYC
	 *
	 * Computing the span part on its own keeps it in the counter's width, so
	 * a 32-bit counter on a 32-bit CPU needs no 64-bit math. Clamp the span
	 * in ticks, before multiplying, so it can't overflow.
	 */
#if TIMER_CORE_COUNTER_WIDTH <= 32
	timer_core_ticks_t span = TIMER_CORE_MAX_SPAN_TICKS;

	if ((ticks <= span) && (timer_core_last_elapsed <= (span - ticks))) {
		span = timer_core_last_elapsed + ticks;
	}
	timer_core_cycles_t offset = (timer_core_cycles_t)span * TIMER_CORE_CYC_PER_TICK;

	if ((TIMER_CORE_MAX_ARM_CYCLES < TIMER_CORE_MAX_UNANNOUNCED_CYCLES) &&
	    (offset > TIMER_CORE_MAX_ARM_CYCLES)) {
		offset = TIMER_CORE_MAX_ARM_CYCLES;
	}
	timer_core_set_compare(timer_core_last_cycle + offset);
#else
	/* 64-bit counter: the math is 64-bit anyway, so compute the deadline
	 * directly and clamp it.
	 */
	uint64_t deadline =
		(timer_core_last_tick + timer_core_last_elapsed + ticks) * TIMER_CORE_CYC_PER_TICK;

	if ((deadline - timer_core_last_cycle) > TIMER_CORE_MAX_ARM_CYCLES) {
		deadline = timer_core_last_cycle + TIMER_CORE_MAX_ARM_CYCLES;
	}
	timer_core_set_compare(deadline);
#endif
#endif
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	/* Deprecated: idle entry now goes through sys_clock_idle_enter(). */
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

/* Announce the whole ticks since the last announce, with the clock lock
 * already held. @p key comes from the driver's sys_clock_lock() and is
 * released here. Use this when the driver has work to do under the same lock
 * first, e.g. adding a wrap to its software count.
 */
static void timer_core_announce_from(k_spinlock_key_t key)
{
	timer_core_ticks_t dticks = timer_core_delta_ticks();

	timer_core_last_cycle += (timer_core_cycles_t)dticks * TIMER_CORE_CYC_PER_TICK;
	timer_core_last_tick += dticks;
	timer_core_last_elapsed = 0;
#if defined(TIMER_CORE_BACKEND_RELOAD)
	/* The armed deadline is used up. The kernel sets the next one, and it
	 * must be written even if it is the same tick.
	 */
	timer_core_armed_deadline = UINT64_MAX;
	timer_core_catchup = false;
#endif

#if !defined(TIMER_CORE_BACKEND_RELOAD)
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Ticking kernel: arm the next tick. A reload counter reloads
		 * by itself.
		 */
		timer_core_arm(1);
	}
#endif

	/* The baseline keeps the full count. Only the value passed to the kernel
	 * is clamped to 32 bits, and that only kicks in after a very long idle
	 * with nothing pending.
	 */
	sys_clock_announce_locked(timer_core_ticks_clamp(dticks), key);
}

/* Announce a number of cycles the driver measured itself, rather than read
 * from the counter, with the clock lock held. @p key comes from the driver's
 * sys_clock_lock() and is released here.
 *
 * For a driver whose counter was stopped while something else kept time,
 * like a low-power companion timer. That time can be longer than the counter
 * can measure, so the math here is always 64-bit. timer_core_announce_from()
 * stays in the counter's width, so only this path pays for it.
 *
 * Only whole ticks are announced. The leftover fraction is still in the
 * driver's counter and is picked up by the next announce.
 *
 * inline so drivers that don't use it get no unused-function warning.
 */
static inline void timer_core_announce_cycles64_from(k_spinlock_key_t key, uint64_t cycles)
{
	uint64_t dticks = cycles / TIMER_CORE_CYC_PER_TICK;

	timer_core_last_cycle += dticks * TIMER_CORE_CYC_PER_TICK;
	timer_core_last_tick += dticks;
	timer_core_last_elapsed = 0;
#if defined(TIMER_CORE_BACKEND_RELOAD)
	timer_core_armed_deadline = UINT64_MAX;
	timer_core_catchup = false;
#endif

	/*
	 * Not timer_core_ticks_clamp(): it takes a timer_core_ticks_t, which
	 * may be too narrow for this count and would truncate it.
	 */
	sys_clock_announce_locked((uint32_t)MIN(dticks, (uint64_t)UINT32_MAX), key);
}

/* Announce the ticks since the last announce. The driver ISR is just a
 * hardware acknowledge followed by this. inline so a driver that uses
 * timer_core_announce_from() instead gets no unused-function warning.
 */
static inline void timer_core_announce(void)
{
	timer_core_announce_from(sys_clock_lock());
}

/*
 * Full 64-bit count, in counter cycles: the baseline plus the cycles since.
 * A driver that scales the count builds its own getters on this.
 *
 * Takes the clock lock when the read isn't atomic:
 *   - TIMER_CORE_COUNTER_NONATOMIC: the count is built in software from
 *     state the ISR also updates;
 *   - a counter narrower than 64 bits: the 64-bit baseline is two loads on a
 *     32-bit CPU.
 *
 * sys_clock_cycle_get_32() avoids the lock for a narrow counter by using only
 * the baseline's low word. That matters because thread usage timing calls it
 * a lot.
 */
static inline uint64_t timer_core_cycle_get(void)
{
#if defined(TIMER_CORE_COUNTER_NONATOMIC) || TIMER_CORE_COUNTER_WIDTH < 64
	k_spinlock_key_t key = sys_clock_lock();
	uint64_t ret = timer_core_last_cycle + timer_core_cycles_since(timer_core_last_cycle);

	sys_clock_unlock(key);
	return ret;
#else
	/* A 64-bit counter is already the full count. */
	return timer_driver_cycle_get();
#endif
}

/*
 * A driver that sets TIMER_CORE_CYCLES_PER_SEC has a counter at another rate
 * than the kernel's, so the core's getters would return the wrong unit. The
 * driver must provide sys_clock_cycle_get_32(), and the 64-bit one if it
 * needs it.
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
	/* Software count: read it under the lock. */
	return (uint32_t)timer_core_cycle_get();
#elif TIMER_CORE_COUNTER_WIDTH < 32
	/* Narrow counter: add the cycles since the baseline's low word. Each is
	 * a single load, and the result is right even if an announce happens
	 * in between, so no lock is needed.
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

/* Switch the baseline to a new counter rate. A driver that changes the timer
 * frequency at run time calls this after rescaling its own counter.
 */
static inline void timer_core_rescale(uint32_t to_hz, uint32_t from_hz)
{
	ARG_UNUSED(from_hz);
#ifdef TIMER_CORE_PRECOMPUTE_CYC_PER_TICK
	timer_core_cyc_per_tick = to_hz / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
#else
	ARG_UNUSED(to_hz);
#endif
	/*
	 * Recompute last_cycle from last_tick, which doesn't depend on the rate,
	 * so it stays on a tick boundary. Scaling the old value instead could
	 * leave it a fraction of a tick off, and the compare arm path could then
	 * underflow.
	 */
	timer_core_last_cycle = timer_core_last_tick * TIMER_CORE_CYC_PER_TICK;
}

/* Arm this CPU's timer one tick past the last announce. An SMP driver calls
 * this from smp_timer_init().
 */
static inline void timer_core_smp_prime(void)
{
#if defined(TIMER_CORE_BACKEND_RELOAD)
	timer_driver_set_reload(TIMER_CORE_CYC_PER_TICK);
#else
	timer_core_set_compare(timer_core_last_cycle + TIMER_CORE_CYC_PER_TICK);
#endif
}

/* Start counting from the current counter value and arm the first tick. The
 * driver calls this from its init, after connecting the IRQ and, with a
 * run-time rate, after setting it.
 */
static inline void timer_core_init(void)
{
#ifndef CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE
	z_sys_clock_lpm_init();
#endif /* !CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE */
#ifdef TIMER_CORE_PRECOMPUTE_CYC_PER_TICK
	/* The rate is known now that the driver has set up the counter. */
	timer_core_cyc_per_tick = TIMER_CORE_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	timer_core_max_span_ticks = TIMER_CORE_MAX_UNANNOUNCED_CYCLES / TIMER_CORE_CYC_PER_TICK;
#endif
#if defined(TIMER_CORE_PRECOMPUTE_CYC_PER_TICK) || defined(TIMER_CORE_CHECK_CYC_PER_TICK_AT_INIT)
	/* The checks a constant rate gets at build time. */
	__ASSERT(TIMER_CORE_CYC_PER_TICK != 0, "timer counter rate is below the tick rate");
	/* Cast both sides to uint64_t: otherwise, with a 64-bit counter, the
	 * compiler warns the comparison is always true.
	 */
	__ASSERT((uint64_t)TIMER_CORE_COUNTER_SAFE_SPAN >= (uint64_t)TIMER_CORE_CYC_PER_TICK,
		 "a tick is longer than the counter can span");
#endif
	/* seed and seed * TIMER_CORE_CYC_PER_TICK are both no larger than the
	 * counter value, so both fit in timer_core_cycles_t.
	 */
	timer_core_cycles_t seed = timer_driver_cycle_get() / TIMER_CORE_CYC_PER_TICK;

	timer_core_last_tick = seed;
	timer_core_last_cycle = seed * TIMER_CORE_CYC_PER_TICK;
	timer_core_last_elapsed = 0;

#if defined(TIMER_CORE_BACKEND_RELOAD)
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Ticking kernel: the driver set the hardware to reload every
		 * tick, and it is never rewritten. Arming here would load the
		 * fraction of a tick left after the seed above, and that would
		 * stick as the period.
		 */
		return;
	}
#endif
	timer_core_arm(1);
}

#endif /* ZEPHYR_DRIVERS_TIMER_SYSTEM_TIMER_GENERIC_H_ */

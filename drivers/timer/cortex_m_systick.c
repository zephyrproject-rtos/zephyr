/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <cmsis_core.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer_lpm.h>

#define COUNTER_MAX 0x00ffffff
#define TIMER_STOPPED 0xff000000

#define SYSTICK_CTRL_CLKSOURCE_MSK_GET()					\
	COND_CODE_1(DT_PROP(DT_NODELABEL(systick), external_clock_source),	\
		    (0), (SysTick_CTRL_CLKSOURCE_Msk))

/* Largest delta we can program into the 24-bit LOAD register. */
#define MAX_CYCLES ((uint32_t)COUNTER_MAX)

/* Minimum reload the driver will program, i.e. the closest-in timeout it can
 * schedule. Two floors apply.
 *
 * Hardware: LOAD is programmed as (cycles - 1), and a LOAD of zero stops the
 * counter (it reloads to zero and never makes another 1->0 transition, so no
 * further interrupt), so the smallest usable value is two cycles (LOAD 1).
 *
 * Masking: it must also exceed the longest time the SysTick interrupt can stay
 * masked. elapsed() reconstructs the current cycle count assuming at most one
 * wrap of the counter has happened since it last ran; it cannot tell one wrap
 * from several. With a tiny LOAD the counter wraps every few cycles, so if
 * interrupts are then held off longer than that LOAD (a long critical section,
 * or a higher-priority ISR that runs for a while) the counter wraps two or
 * more times before the SysTick ISR can account for it. Each unaccounted wrap
 * silently drops a full LOAD's worth of cycles: the software clock falls
 * permanently behind real time and every pending timeout fires late by that
 * much. A floor above the worst-case masking window guarantees at most one
 * wrap between reads, which elapsed() does handle.
 *
 * That is a wall-clock budget, so express it as a fixed time converted to
 * cycles at the actual frequency (k_us_to_cyc_ceil32() follows the runtime
 * rate where the timer reports it), computed at init. A fixed cycle count is
 * meaningless across clock rates: the old 1024-cycle floor is ~10 us at
 * 100 MHz but ~31 ms (31 ticks!) on a 32 kHz SysTick. The tick rate is
 * deliberately not involved; if the resulting value exceeds one tick on some
 * clock, sub-tick timeouts are simply unavailable there.
 *
 * Keep it as small as correctness allows, not as large as a tick would
 * permit: this is a floor for safe rescheduling, not a jitter control.
 * Inflating it does trim the short-period tail, but it eats sub-tick headroom,
 * and as it approaches CYC_PER_TICK the driver can no longer place a timeout
 * inside the current tick, so the small per-ISR surplus is carried over and
 * two ticks get announced at once (a double expiry). 10 us is under a cycle at
 * 32 kHz, where the two-cycle hardware floor takes over; either way it stays
 * far below CYC_PER_TICK.
 *
 * A device tree "zephyr,min-timeout-cycles" property still overrides the
 * budget, subject to the same two-cycle hardware floor.
 */
#define SYSTICK_MIN_DELAY_US 10U

static inline uint32_t systick_min_delay(void)
{
	uint32_t override_cyc =
		DT_PROP_OR(DT_NODELABEL(systick), zephyr_min_timeout_cycles, 0U);
	uint32_t cyc = (override_cyc != 0U) ? override_cyc
					    : k_us_to_cyc_ceil32(SYSTICK_MIN_DELAY_US);

	/* Floor at two cycles: LOAD is programmed as (cycles - 1) and a LOAD
	 * of zero stops the counter. This is the binding floor on a slow clock
	 * (e.g. 32 kHz, where the 10 us budget rounds below it).
	 */
	return MAX(2U, cyc);
}

static uint32_t last_load;

/* Minimum LOAD value; derived from the cycle rate in sys_clock_driver_init()
 * (and refreshed on a runtime frequency change). See systick_min_delay().
 */
static uint32_t min_delay;

/*
 * This local variable holds the amount of SysTick HW cycles elapsed
 * and it is updated in sys_clock_isr() and timer_driver_set_reload().
 *
 * Note:
 *  At an arbitrary point in time the "current" value of the SysTick
 *  HW timer is calculated as:
 *
 * t = cycle_count + elapsed();
 */
static uint32_t cycle_count;

/*
 * This local variable holds the amount of elapsed HW cycles due to
 * SysTick timer wraps ('overflows') and is used in the calculation
 * in elapsed() function, as well as in the updates to cycle_count.
 *
 * Note:
 * Each time cycle_count is updated with the value from overflow_cyc,
 * the overflow_cyc must be reset to zero.
 */
static volatile uint32_t overflow_cyc;

#if !defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE)
/* This local variable indicates that the timeout was set right before
 * entering idle state.
 *
 * It is used for chips that has to use a separate idle timer in such
 * case because the Cortex-m SysTick is not clocked in the low power
 * mode state.
 */
static bool timeout_idle;

#if !defined(CONFIG_SYSTEM_TIMER_RESET_BY_LPM)
/* Cycle counter before entering the idle state. */
static uint32_t cycle_pre_idle;
#endif /* !CONFIG_SYSTEM_TIMER_RESET_BY_LPM */
#endif /* !CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE */

/* This internal function calculates the amount of HW cycles that have
 * elapsed since the last time the absolute HW cycles counter has been
 * updated. 'cycle_count' may be updated either by the ISR, or when we
 * re-program the SysTick.LOAD register, in timer_driver_set_reload().
 *
 * Additionally, the function updates the 'overflow_cyc' counter, that
 * holds the amount of elapsed HW cycles due to (possibly) multiple
 * timer wraps (overflows).
 *
 * @param val_out Optional pointer to store the raw SysTick->VAL snapshot
 *                (val2, as read, before wrap-realignment) used in the
 *                calculation, so a caller needing that raw value gets it
 *                with no gap after the measurement (see
 *                timer_driver_set_reload()).
 *
 * Prerequisites:
 * - reprogramming of SysTick.LOAD must be clearing the SysTick.COUNTER
 *   register and the 'overflow_cyc' counter.
 * - ISR must be clearing the 'overflow_cyc' counter.
 * - no more than one counter-wrap has occurred between
 *     - the timer reset or the last time the function was called
 *     - and until the current call of the function is completed.
 * - the function is invoked with interrupts disabled.
 */
static uint32_t elapsed(uint32_t *val_out)
{
	uint32_t val1 = SysTick->VAL;	/* A */
	uint32_t ctrl = SysTick->CTRL;	/* B */
	uint32_t val2 = SysTick->VAL;	/* C */

	/* SysTick behavior: The counter wraps after zero automatically.
	 * The COUNTFLAG field of the CTRL register is set when it
	 * decrements from 1 to 0. Reading the control register
	 * automatically clears that field. When a timer is started,
	 * count begins at zero then wraps after the first cycle.
	 * Reference:
	 *  Armv6-m (B3.3.1) https://developer.arm.com/documentation/ddi0419
	 *  Armv7-m (B3.3.1) https://developer.arm.com/documentation/ddi0403
	 *  Armv8-m (B11.1)  https://developer.arm.com/documentation/ddi0553
	 *
	 * First, manually wrap/realign val1 and val2 from [0:last_load-1]
	 * to [1:last_load]. This allows subsequent code to assume that
	 * COUNTFLAG and wrapping occur on the same cycle.
	 *
	 * If the count wrapped...
	 * 1) Before A then COUNTFLAG will be set and val1 >= val2
	 * 2) Between A and B then COUNTFLAG will be set and val1 < val2
	 * 3) Between B and C then COUNTFLAG will be clear and val1 < val2
	 * 4) After C we'll see it next time
	 *
	 * So the count in val2 is post-wrap and last_load needs to be
	 * added if and only if COUNTFLAG is set or val1 < val2.
	 */
	if (val_out != NULL) {
		*val_out = val2;
	}

	if (val1 == 0) {
		val1 = last_load;
	}
	if (val2 == 0) {
		val2 = last_load;
	}

	if ((ctrl & SysTick_CTRL_COUNTFLAG_Msk)
	    || (val1 < val2)) {
		overflow_cyc += last_load;

		/* We know there was a wrap, but we might not have
		 * seen it in CTRL, so clear it. */
		(void)SysTick->CTRL;
	}

	return (last_load - val2) + overflow_cyc;
}

static inline uint32_t timer_driver_cycle_get(void)
{
	return cycle_count + elapsed(NULL);
}

static void timer_driver_set_reload(uint32_t cycles)
{
	/*
	 * elapsed() returns its own SysTick->VAL snapshot (val2) through val1, so
	 * the window measured by (val1 - val2) at the bottom abuts the window
	 * already accounted for by elapsed() with no gap: reading SysTick->VAL
	 * separately after elapsed() would leave the function-return cycles
	 * counted by neither, i.e. systematically lost drift. The core has
	 * already clamped 'cycles' to [MIN_DELAY, MAX_CYCLES].
	 */
	uint32_t val1;
	uint32_t pending = elapsed(&val1);
	uint32_t old_load = last_load;

	cycle_count += pending;
	overflow_cyc = 0U;

	/*
	 * val2 must be sampled while the OLD LOAD is still the reload source: if
	 * a wrap happened between SysTick->LOAD being reprogrammed and the val2
	 * read, VAL would reload from the NEW LOAD and the drift-comp formula
	 * below (which uses old_load for the wrap case) would be wrong. Updating
	 * last_load (a software shadow) is HW-inert and safe to do before val2.
	 *
	 * COUNTFLAG is not checked here: the caller guarantees this runs faster
	 * than MIN_DELAY cycles, so a wrap cannot be missed.
	 */
	last_load = cycles;

	uint32_t val2 = SysTick->VAL;

	SysTick->LOAD = cycles - 1U;
	SysTick->VAL = 0U;	/* resets counter, clears COUNTFLAG */

	/*
	 * Clear any pending SysTick exception from the old schedule. Writing
	 * VAL=0 clears COUNTFLAG in CTRL but not ICSR.PENDSTSET, so without this
	 * a wrap that fired just before the reprogram would still trigger the
	 * ISR once interrupts are re-enabled. On Armv8-M, preserve STTNS (R/W)
	 * while writing the W1C bit.
	 */
#ifdef SCB_ICSR_STTNS_Msk
	SCB->ICSR = (SCB->ICSR & SCB_ICSR_STTNS_Msk) | SCB_ICSR_PENDSTCLR_Msk;
#else
	SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
#endif

	if (val1 < val2) {
		cycle_count += val1 + (old_load - val2);
	} else {
		cycle_count += val1 - val2;
	}
}

/*
 * SysTick is an auto-reload down-counter: a RELOAD backend. LOAD bounds the arm
 * range, and min_delay is the reload floor, the closest-in timeout that can be
 * programmed and still be caught.
 *
 * LOAD is reprogrammed to a different value on every arm in tickless mode, so
 * the raw register is a variable-period sawtooth rather than the free-running
 * monotonic count the core masks and extends for a plain narrow counter. Hence
 * the software count: timer_driver_cycle_get() returns cycle_count, the whole
 * periods accumulated on each wrap and reprogram, plus the current partial.
 * That is not a register read either, and cycle_count and elapsed() are shared
 * with the ISR and the reprogram path, so the core takes the clock lock around
 * the public cycle getters.
 *
 * 32 bits is the whole of that count. The core extends it from its own announce
 * baseline, so sys_clock_cycle_get_64() needs no wider accumulator here, and the
 * 24-bit LOAD keeps the span between announces well inside 32 bits. The one span
 * that is not is a low-power sleep, which sys_clock_idle_exit() hands over
 * separately rather than making every path pay for the width.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_COUNTER_WIDTH 32
#define TIMER_CORE_ALARM_MIN_CYCLES min_delay
#define TIMER_CORE_ALARM_MAX_CYCLES MAX_CYCLES
#define TIMER_CORE_COUNTER_NONATOMIC

#include "system_timer_generic.h"

/* sys_clock_isr is calling directly from the platform's vectors table.
 * However using ISR_DIRECT_DECLARE() is not so suitable due to possible
 * tracing overflow, so here is a stripped down version of it.
 */
ARCH_ISR_DIAG_OFF
__attribute__((interrupt("IRQ"))) void sys_clock_isr(void)
{
#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_enter();
#endif /* CONFIG_TRACING_ISR */

	k_spinlock_key_t key = sys_clock_lock();

	/* Commit the wrap that fired this interrupt into cycle_count so
	 * timer_driver_cycle_get() (and therefore the announce) sees it. Done under
	 * the clock lock, atomically with the announce that follows, so a
	 * higher-priority interrupt reading the cycle counter never observes a
	 * half-committed overflow.
	 */
	elapsed(NULL);
	cycle_count += overflow_cyc;
	overflow_cyc = 0;

#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER)
	/* Rare case, when the interrupt was triggered, with previously programmed
	 * LOAD value, just before entering the idle mode (SysTick is clocked) or right
	 * after exiting the idle mode, before executing the procedure in the
	 * sys_clock_idle_exit function.
	 */
	if (timeout_idle) {
		sys_clock_unlock(key);
		ISR_DIRECT_PM();
		z_arm_int_exit();

		return;
	}
#endif /* CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER */

	timer_core_announce_from(key);

	ISR_DIRECT_PM();

#ifdef CONFIG_TRACING_ISR
	sys_trace_isr_exit();
#endif /* CONFIG_TRACING_ISR */

	z_arm_int_exit();
}
ARCH_ISR_DIAG_ON

#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
void z_sys_clock_hw_cycles_per_sec_update(uint32_t new_hz)
{
	extern unsigned int z_clock_hw_cycles_per_sec;
	uint32_t old_hz = (uint32_t)z_clock_hw_cycles_per_sec;

	if ((old_hz == 0U) || (new_hz == 0U) || (old_hz == new_hz)) {
		return;
	}

	k_spinlock_key_t key = sys_clock_lock();

	/* Publish the new frequency. */
	z_clock_hw_cycles_per_sec = new_hz;

	/* The floor is a wall-clock budget, so re-derive it at the new rate. */
	min_delay = systick_min_delay();

	uint32_t load_old = last_load;

	if (load_old != TIMER_STOPPED) {
		cycle_count += elapsed(NULL);
		overflow_cyc = 0U;
	}

	/* Rescale the synthesized counter and the core's announce baseline from
	 * old cycles to new cycles so the tick accounting stays continuous.
	 */
	cycle_count = (uint32_t)(((uint64_t)cycle_count * (uint64_t)new_hz) / (uint64_t)old_hz);
	timer_core_rescale(new_hz, old_hz);

	if (load_old != TIMER_STOPPED) {
		uint32_t new_load;

		if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
			uint32_t val = SysTick->VAL;

			if (val == 0U) {
				val = load_old;
			}

			new_load = (uint32_t)(((uint64_t)val * (uint64_t)new_hz) /
					      (uint64_t)old_hz);
		} else {
			/*
			 * Tickful: LOAD is one tick and is never otherwise
			 * reprogrammed, so load_old already holds one tick at
			 * the old rate. Scale it like the tickless branch; the
			 * rescale is a pure cycle-domain operation, with no need
			 * for the cycles-per-tick value.
			 */
			new_load = (uint32_t)(((uint64_t)load_old * (uint64_t)new_hz) /
					      (uint64_t)old_hz);
		}

		new_load = MAX(new_load, min_delay);
		if (new_load > COUNTER_MAX) {
			new_load = COUNTER_MAX;
		}

		last_load = new_load;
		SysTick->LOAD = new_load - 1U;
		SysTick->VAL = 0U;
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	}

	sys_clock_unlock(key);
}
#endif /* CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE */

void sys_clock_idle_enter(uint32_t ticks)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE)
	if (ticks != SYS_CLOCK_IDLE_FOREVER) {
		return;
	}

	/* Nothing to wake up for and the uptime may drift, so stop the counter
	 * outright rather than let a fast CPU and a 24-bit LOAD wake this CPU
	 * several times a second for nothing. sys_clock_idle_exit() starts it
	 * again on the way back.
	 */
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	last_load = TIMER_STOPPED;
#else
	/* SYS_CLOCK_IDLE_FOREVER converts to a wakeup tens of days out, which
	 * is the intent: the companion may wake earlier, never later.
	 */
	uint64_t timeout_us = k_ticks_to_us_ceil64(ticks);

	timeout_idle = true;

	/**
	 * Invoke platform-specific layer to configure LPTIM
	 * such that system wakes up after timeout elapses.
	 */
	z_sys_clock_lpm_enter(timeout_us);

#if !defined(CONFIG_SYSTEM_TIMER_RESET_BY_LPM)
	/* Store current value of SysTick counter to be able to
	 * calculate a difference in measurements after exiting
	 * the low-power state.
	 */
	cycle_pre_idle = cycle_count + elapsed(NULL);
#else /* CONFIG_SYSTEM_TIMER_RESET_BY_LPM */
	/**
	 * SysTick will be placed under reset once we enter
	 * low-power mode. Turn it off right now then update
	 * the cycle counter now, since we won't be able to
	 * to it after waking up.
	 */
	sys_clock_disable();
	/* Ensure the SysTick interrupt is not pending. This is safe
	 * as we just did the ISR's job, and MUST be done because
	 * a pending interrupt could inhibit low-power mode entry.
	 * Note: On Armv8-M, ICSR.STTNS is R/W, so preserve it while
	 * writing the write-1-to-clear PENDSTCLR bit.
	 */
#ifdef SCB_ICSR_STTNS_Msk
	SCB->ICSR = (SCB->ICSR & SCB_ICSR_STTNS_Msk) | SCB_ICSR_PENDSTCLR_Msk;
#else
	SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
#endif

	cycle_count += elapsed(NULL);
	overflow_cyc = 0;
#endif /* !CONFIG_SYSTEM_TIMER_RESET_BY_LPM */
#endif /* CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE */
}

void sys_clock_idle_exit(void)
{
#if !defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE)
	if (timeout_idle) {
		k_spinlock_key_t key = sys_clock_lock();
		uint32_t systick_diff;
		uint64_t systick_us, idle_timer_us, missed_cycles;

#if !defined(CONFIG_SYSTEM_TIMER_RESET_BY_LPM)
		/**
		 * Get current value for SysTick and calculate how
		 * much time has passed since last measurement.
		 */
		systick_diff = cycle_count + elapsed(NULL) - cycle_pre_idle;
		systick_us = k_cyc_to_us_floor64(systick_diff);
#else /* CONFIG_SYSTEM_TIMER_RESET_BY_LPM */
		/* SysTick was placed under reset so it didn't tick */
		systick_diff = systick_us = 0;
#endif /* !CONFIG_SYSTEM_TIMER_RESET_BY_LPM */

		/**
		 * Query platform-specific code for elapsed time according to LPTIM.
		 */
		idle_timer_us = z_sys_clock_lpm_exit();

		/* Calculate difference in measurements to get how much time
		 * the SysTick missed in idle state.
		 */
		if (idle_timer_us < systick_us) {
			/* This case is possible, when the time in low power mode is
			 * very short or 0. SysTick usually has higher measurement
			 * resolution of than the IDLE timer, thus the measurement of
			 * passed time since the sys_clock_set_timeout call can be higher.
			 */
			missed_cycles = 0;
		} else {
			uint64_t measurement_diff_us;

			measurement_diff_us = idle_timer_us - systick_us;
			missed_cycles = k_us_to_cyc_floor64(measurement_diff_us);
		}

		/*
		 * A sleep can outrun the 32-bit count, so hand the span to the core
		 * directly rather than routing it through the counter. Only this
		 * path pays for the wider arithmetic.
		 *
		 * The sleep alone: what was already un-announced when the CPU went
		 * idle stays in the counter, as does the sub-tick remainder, and the
		 * next announce picks both up.
		 */
		cycle_count += (uint32_t)missed_cycles;
		timeout_idle = false;

		timer_core_announce_cycles64_from(key, missed_cycles);
	}
#endif /* !CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE */

	if (last_load == TIMER_STOPPED || IS_ENABLED(CONFIG_SYSTEM_TIMER_RESET_BY_LPM)) {
		/* SysTick was stopped or placed under reset.
		 * Restart the timer from scratch.
		 */
		k_spinlock_key_t key = sys_clock_lock();

		last_load = TIMER_CORE_CYC_PER_TICK;
		SysTick->LOAD = last_load - 1;
		SysTick->VAL = 0; /* resets timer to last_load */
		if (!IS_ENABLED(CONFIG_SYSTEM_TIMER_RESET_BY_LPM)) {
			SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
		} else {
			NVIC_SetPriority(SysTick_IRQn, _IRQ_PRIO_OFFSET);
			SysTick->CTRL |= (SysTick_CTRL_ENABLE_Msk |
					  SysTick_CTRL_TICKINT_Msk |
					  SYSTICK_CTRL_CLKSOURCE_MSK_GET());
		}

		/* The hardware no longer holds the deadline the core last
		 * armed, so announce here rather than just unlocking: that
		 * resyncs the kernel and drops the stale arm, which would
		 * otherwise let the next sys_clock_set_timeout() land on the
		 * same tick and skip reprogramming.
		 */
		timer_core_announce_from(key);
	}
}

void sys_clock_disable(void)
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

static int sys_clock_driver_init(void)
{

	NVIC_SetPriority(SysTick_IRQn, _IRQ_PRIO_OFFSET);
	min_delay = systick_min_delay();
	last_load = TIMER_CORE_CYC_PER_TICK;
	overflow_cyc = 0U;
	SysTick->LOAD = last_load - 1;
	SysTick->VAL = 0; /* resets timer to last_load */

	uint32_t ctrl_flags = SysTick_CTRL_ENABLE_Msk |
			      SysTick_CTRL_TICKINT_Msk |
			      SYSTICK_CTRL_CLKSOURCE_MSK_GET();

	SysTick->CTRL = ctrl_flags;

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

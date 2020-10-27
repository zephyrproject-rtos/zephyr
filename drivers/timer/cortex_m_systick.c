/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#define COUNTER_MAX 0x00ffffff
#define TIMER_STOPPED 0xff000000

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

/* Minimum cycles in the future to try to program.  Note that this is
 * NOT simply "enough cycles to get the counter read and reprogrammed
 * reliably" -- it becomes the minimum value of the LOAD register, and
 * thus reflects how much time we can reliably see expire between
 * calls to elapsed() to read the COUNTFLAG bit.  So it needs to be
 * set to be larger than the maximum time the interrupt might be
 * masked.  Choosing a fraction of a tick is probably a good enough
 * default, with an absolute minimum of 1k cyc.
 */
#define MIN_DELAY MAX(1024, (CYC_PER_TICK/16))

#define TICKLESS (IS_ENABLED(CONFIG_TICKLESS_KERNEL))

static struct k_spinlock lock;

static uint32_t last_load;

/*
 * This local variable holds the amount of SysTick HW cycles elapsed
 * and it is updated in z_clock_isr() and z_clock_set_timeout().
 *
 * Note:
 *  At an arbitrary point in time the "current" value of the SysTick
 *  HW timer is calculated as:
 *
 * t = cycle_counter + elapsed();
 */
static uint32_t cycle_count;

/*
 * This local variable holds the amount of elapsed SysTick HW cycles
 * that have been announced to the kernel.
 */
static uint32_t announced_cycles;

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

/* This internal function calculates the amount of HW cycles that have
 * elapsed since the last time the absolute HW cycles counter has been
 * updated. 'cycle_count' may be updated either by the ISR, or when we
 * re-program the SysTick.LOAD register, in z_clock_set_timeout().
 *
 * Additionally, the function updates the 'overflow_cyc' counter, that
 * holds the amount of elapsed HW cycles due to (possibly) multiple
 * timer wraps (overflows).
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
static uint32_t elapsed(void)
{
	uint32_t val1 = SysTick->VAL;	/* A */
	uint32_t ctrl = SysTick->CTRL;	/* B */
	uint32_t val2 = SysTick->VAL;	/* C */

	/* SysTick behavior: The counter wraps at zero automatically,
	 * setting the COUNTFLAG field of the CTRL register when it
	 * does.  Reading the control register automatically clears
	 * that field.
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
	if ((ctrl & SysTick_CTRL_COUNTFLAG_Msk)
	    || (val1 < val2)) {
		overflow_cyc += last_load;

		/* We know there was a wrap, but we might not have
		 * seen it in CTRL, so clear it. */
		(void)SysTick->CTRL;
	}

	return (last_load - val2) + overflow_cyc;
}

/* Callout out of platform assembly, not hooked via IRQ_CONNECT... */
void z_clock_isr(void *arg)
{
	ARG_UNUSED(arg);
	uint32_t dticks;

	/* Update overflow_cyc and clear COUNTFLAG by invoking elapsed() */
	elapsed();

	/* Increment the amount of HW cycles elapsed (complete counter
	 * cycles) and announce the progress to the kernel.
	 */
	cycle_count += overflow_cyc;
	overflow_cyc = 0;

	if (TICKLESS) {
		/* In TICKLESS mode, the SysTick.LOAD is re-programmed
		 * in z_clock_set_timeout(), followed by resetting of
		 * the counter (VAL = 0).
		 *
		 * If a timer wrap occurs right when we re-program LOAD,
		 * the ISR is triggered immediately after z_clock_set_timeout()
		 * returns; in that case we shall not increment the cycle_count
		 * because the value has been updated before LOAD re-program.
		 *
		 * We can assess if this is the case by inspecting COUNTFLAG.
		 */

		dticks = (cycle_count - announced_cycles) / CYC_PER_TICK;
		announced_cycles += dticks * CYC_PER_TICK;
		z_clock_announce(dticks);
	} else {
		z_clock_announce(1);
	}
	z_arm_int_exit();
}

int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);

	NVIC_SetPriority(SysTick_IRQn, _IRQ_PRIO_OFFSET);
	last_load = CYC_PER_TICK - 1;
	overflow_cyc = 0U;
	SysTick->LOAD = last_load;
	SysTick->VAL = 0; /* resets timer to last_load */
	SysTick->CTRL |= (SysTick_CTRL_ENABLE_Msk |
			  SysTick_CTRL_TICKINT_Msk |
			  SysTick_CTRL_CLKSOURCE_Msk);
	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	/* Fast CPUs and a 24 bit counter mean that even idle systems
	 * need to wake up multiple times per second.  If the kernel
	 * allows us to miss tick announcements in idle, then shut off
	 * the counter. (Note: we can assume if idle==true that
	 * interrupts are already disabled)
	 */
	if (IS_ENABLED(CONFIG_TICKLESS_IDLE) && idle
	    && ticks == K_TICKS_FOREVER) {
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		last_load = TIMER_STOPPED;
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	uint32_t delay;

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t pending = elapsed();

	cycle_count += pending;
	overflow_cyc = 0U;

	uint32_t unannounced = cycle_count - announced_cycles;

	if ((int32_t)unannounced < 0) {
		/* We haven't announced for more than half the 32-bit
		 * wrap duration, because new timeouts keep being set
		 * before the existing one fires.  Force an announce
		 * to avoid loss of a wrap event, making sure the
		 * delay is at least the minimum delay possible.
		 */
		last_load = MIN_DELAY;
	} else {
		/* Desired delay in the future */
		delay = ticks * CYC_PER_TICK;

		/* Round delay up to next tick boundary */
		delay += unannounced;
		delay =
		 ((delay + CYC_PER_TICK - 1) / CYC_PER_TICK) * CYC_PER_TICK;
		delay -= unannounced;
		delay = MAX(delay, MIN_DELAY);
		if (delay > MAX_CYCLES) {
			last_load = MAX_CYCLES;
		} else {
			last_load = delay;
		}
	}
	SysTick->LOAD = last_load - 1;
	SysTick->VAL = 0; /* resets timer to last_load */

	k_spin_unlock(&lock, key);
#endif
}

uint32_t z_clock_elapsed(void)
{
	if (!TICKLESS) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t cyc = elapsed() + cycle_count - announced_cycles;

	k_spin_unlock(&lock, key);
	return cyc / CYC_PER_TICK;
}

uint32_t z_timer_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = elapsed() + cycle_count;

	k_spin_unlock(&lock, key);
	return ret;
}

void z_clock_idle_exit(void)
{
	if (last_load == TIMER_STOPPED) {
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	}
}

void sys_clock_disable(void)
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <cmsis_core.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/counter.h>

#define COUNTER_MAX 0x00ffffff
#define TIMER_STOPPED 0xff000000

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((k_ticks_t)(COUNTER_MAX / CYC_PER_TICK) - 1)
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
#define MIN_DELAY MAX(1024U, ((uint32_t)CYC_PER_TICK/16U))

#define TICKLESS (IS_ENABLED(CONFIG_TICKLESS_KERNEL))

static struct k_spinlock lock;

static uint32_t last_load;

#ifdef CONFIG_CORTEX_M_SYSTICK_64BIT_CYCLE_COUNTER
#define cycle_t uint64_t
#else
#define cycle_t uint32_t
#endif

/*
 * This local variable holds the amount of SysTick HW cycles elapsed
 * and it is updated in sys_clock_isr() and sys_clock_set_timeout().
 *
 * Note:
 *  At an arbitrary point in time the "current" value of the SysTick
 *  HW timer is calculated as:
 *
 * t = cycle_counter + elapsed();
 */
static cycle_t cycle_count;

/*
 * This local variable holds the amount of elapsed SysTick HW cycles
 * that have been announced to the kernel.
 *
 * Note:
 * Additions/subtractions/comparisons of 64-bits values on 32-bits systems
 * are very cheap. Divisions are not. Make sure the difference between
 * cycle_count and announced_cycles is stored in a 32-bit variable before
 * dividing it by CYC_PER_TICK.
 */
static cycle_t announced_cycles;

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

#ifdef CONFIG_CORTEX_M_SYSTICK_IDLE_TIMER
/* This local variable indicates that the timeout was set right before
 * entering idle state.
 *
 * It is used for chips that has to use a separate idle timer in such
 * case because the Cortex-m SysTick is not clocked in the low power
 * mode state.
 */
static bool timeout_idle;

/* Cycle counter before entering the idle state. */
static cycle_t cycle_pre_idle;

/* Idle timer value before entering the idle state. */
static uint32_t idle_timer_pre_idle;

/* Idle timer used for timer while entering the idle state */
static const struct device *idle_timer = DEVICE_DT_GET(DT_CHOSEN(zephyr_cortex_m_idle_timer));
#endif /* CONFIG_CORTEX_M_SYSTICK_IDLE_TIMER */

/* This internal function calculates the amount of HW cycles that have
 * elapsed since the last time the absolute HW cycles counter has been
 * updated. 'cycle_count' may be updated either by the ISR, or when we
 * re-program the SysTick.LOAD register, in sys_clock_set_timeout().
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

/* Callout out of platform assembly, not hooked via IRQ_CONNECT... */
void sys_clock_isr(void *arg)
{
	ARG_UNUSED(arg);
	uint32_t dcycles;
	uint32_t dticks;

	/* Update overflow_cyc and clear COUNTFLAG by invoking elapsed() */
	elapsed();

	/* Increment the amount of HW cycles elapsed (complete counter
	 * cycles) and announce the progress to the kernel.
	 */
	cycle_count += overflow_cyc;
	overflow_cyc = 0;

#ifdef CONFIG_CORTEX_M_SYSTICK_IDLE_TIMER
	/* Rare case, when the interrupt was triggered, with previously programmed
	 * LOAD value, just before entering the idle mode (SysTick is clocked) or right
	 * after exiting the idle mode, before executing the procedure in the
	 * sys_clock_idle_exit function.
	 */
	if (timeout_idle) {
		z_arm_int_exit();

		return;
	}
#endif /* CONFIG_CORTEX_M_SYSTICK_IDLE_TIMER */

	if (TICKLESS) {
		/* In TICKLESS mode, the SysTick.LOAD is re-programmed
		 * in sys_clock_set_timeout(), followed by resetting of
		 * the counter (VAL = 0).
		 *
		 * If a timer wrap occurs right when we re-program LOAD,
		 * the ISR is triggered immediately after sys_clock_set_timeout()
		 * returns; in that case we shall not increment the cycle_count
		 * because the value has been updated before LOAD re-program.
		 *
		 * We can assess if this is the case by inspecting COUNTFLAG.
		 */

		dcycles = cycle_count - announced_cycles;
		dticks = dcycles / CYC_PER_TICK;
		announced_cycles += dticks * CYC_PER_TICK;
		sys_clock_announce(dticks);
	} else {
		sys_clock_announce(1);
	}
	z_arm_int_exit();
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	/* Fast CPUs and a 24 bit counter mean that even idle systems
	 * need to wake up multiple times per second.  If the kernel
	 * allows us to miss tick announcements in idle, then shut off
	 * the counter. (Note: we can assume if idle==true that
	 * interrupts are already disabled)
	 */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL) && idle && ticks == K_TICKS_FOREVER) {
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		last_load = TIMER_STOPPED;
		return;
	}

#ifdef CONFIG_CORTEX_M_SYSTICK_IDLE_TIMER
	if (idle) {
		uint64_t timeout_us =
			((uint64_t)ticks * USEC_PER_SEC) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
		struct counter_alarm_cfg cfg = {
			.callback = NULL,
			.ticks = counter_us_to_ticks(idle_timer, timeout_us),
			.user_data = NULL,
			.flags = 0,
		};

		timeout_idle = true;

		/* Set the alarm using timer that runs the idle.
		 * Needed rump-up/setting time, lower accurency etc. should be
		 * included in the exit-latency in the power state definition.
		 */
		counter_cancel_channel_alarm(idle_timer, 0);
		counter_set_channel_alarm(idle_timer, 0, &cfg);

		/* Store current values to calculate a difference in
		 * measurements after exiting the idle state.
		 */
		counter_get_value(idle_timer, &idle_timer_pre_idle);
		cycle_pre_idle = cycle_count + elapsed();

		return;
	}
#endif /* CONFIG_CORTEX_M_SYSTICK_IDLE_TIMER */

#if defined(CONFIG_TICKLESS_KERNEL)
	uint32_t delay;
	uint32_t val1, val2;
	uint32_t last_load_ = last_load;

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t pending = elapsed();

	val1 = SysTick->VAL;

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
		delay = DIV_ROUND_UP(delay, CYC_PER_TICK) * CYC_PER_TICK;
		delay -= unannounced;
		delay = MAX(delay, MIN_DELAY);
		if (delay > MAX_CYCLES) {
			last_load = MAX_CYCLES;
		} else {
			last_load = delay;
		}
	}

	val2 = SysTick->VAL;

	SysTick->LOAD = last_load - 1;
	SysTick->VAL = 0; /* resets timer to last_load */

	/*
	 * Add elapsed cycles while computing the new load to cycle_count.
	 *
	 * Note that comparing val1 and val2 is normaly not good enough to
	 * guess if the counter wrapped during this interval. Indeed if val1 is
	 * close to LOAD, then there are little chances to catch val2 between
	 * val1 and LOAD after a wrap. COUNTFLAG should be checked in addition.
	 * But since the load computation is faster than MIN_DELAY, then we
	 * don't need to worry about this case.
	 */
	if (val1 < val2) {
		cycle_count += (val1 + (last_load_ - val2));
	} else {
		cycle_count += (val1 - val2);
	}
	k_spin_unlock(&lock, key);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!TICKLESS) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t unannounced = cycle_count - announced_cycles;
	uint32_t cyc = elapsed() + unannounced;

	k_spin_unlock(&lock, key);
	return cyc / CYC_PER_TICK;
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = cycle_count;

	ret += elapsed();
	k_spin_unlock(&lock, key);
	return ret;
}

#ifdef CONFIG_CORTEX_M_SYSTICK_64BIT_CYCLE_COUNTER
uint64_t sys_clock_cycle_get_64(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t ret = cycle_count + elapsed();

	k_spin_unlock(&lock, key);
	return ret;
}
#endif

void sys_clock_idle_exit(void)
{
#ifdef CONFIG_CORTEX_M_SYSTICK_IDLE_TIMER
	if (timeout_idle) {
		cycle_t systick_diff, missed_cycles;
		uint32_t idle_timer_diff, idle_timer_post, dcycles, dticks;
		uint64_t systick_us, idle_timer_us;

		/* Get current values for both timers */
		counter_get_value(idle_timer, &idle_timer_post);
		systick_diff = cycle_count + elapsed() - cycle_pre_idle;

		/* Calculate has much time has pasted since last measurement for both timers */
		/* Check IDLE timer overflow */
		if (idle_timer_pre_idle > idle_timer_post) {
			idle_timer_diff =
				(counter_get_top_value(idle_timer) - idle_timer_pre_idle) +
				idle_timer_post + 1;

		} else {
			idle_timer_diff = idle_timer_post - idle_timer_pre_idle;
		}
		idle_timer_us = counter_ticks_to_us(idle_timer, idle_timer_diff);
		systick_us =
			((uint64_t)systick_diff * USEC_PER_SEC) / sys_clock_hw_cycles_per_sec();

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
			missed_cycles = (sys_clock_hw_cycles_per_sec() * measurement_diff_us) /
					USEC_PER_SEC;
		}

		/* Update the cycle counter to include the cycles missed in idle */
		cycle_count += missed_cycles;

		/* Announce the passed ticks to the kernel */
		dcycles = cycle_count + elapsed() - announced_cycles;
		dticks = dcycles / CYC_PER_TICK;
		announced_cycles += dticks * CYC_PER_TICK;
		sys_clock_announce(dticks);

		/* We've alredy performed all needed operations */
		timeout_idle = false;
	}
#endif /* CONFIG_CORTEX_M_SYSTICK_IDLE_TIMER */

	if (last_load == TIMER_STOPPED) {
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	}
}

void sys_clock_disable(void)
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

static int sys_clock_driver_init(void)
{

	NVIC_SetPriority(SysTick_IRQn, _IRQ_PRIO_OFFSET);
	last_load = CYC_PER_TICK;
	overflow_cyc = 0U;
	SysTick->LOAD = last_load - 1;
	SysTick->VAL = 0; /* resets timer to last_load */
	SysTick->CTRL |= (SysTick_CTRL_ENABLE_Msk |
			  SysTick_CTRL_TICKINT_Msk |
			  SysTick_CTRL_CLKSOURCE_Msk);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

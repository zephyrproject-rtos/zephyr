/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#define COUNTER_MAX	GENMASK(23, 0)
#define TIMER_STOPPED	(~COUNTER_MAX)

#define SYST_BASE	((mem_addr_t)DT_REG_ADDR(DT_NODELABEL(systick)))

/* SysTick Control and Status Register
 *
 * The SysTick SYST_CSR register enables the SysTick features.
 * The register resets to 0x00000000, or to 0x00000004 if your device does not
 * implement a reference clock.
 */
#define SYST_CSR	SYST_BASE
#define SYST_CSR_COUNTFLAG	BIT(16)
#define SYST_CSR_CLKSOURCE	BIT(2)
#define SYST_CSR_TICKINT	BIT(1)
#define SYST_CSR_ENABLE		BIT(0)

/* SysTick Reload Value Register
 *
 *  The SYST_RVR register specifies the start value to load into the
 *  SYST_CVR register.
 */
#define SYST_RVR	(SYST_BASE + 0x4)

/* SysTick Current Value Register
 *
 * The SYST_CVR register contains the current value of the SysTick counter.
 */
#define SYST_CVR	(SYST_BASE + 0x8)

/*TODO: should be used in time calculation, in case of external clock usage */
#define SYST_CALIB	(SYST_BASE + 0xc)
#define SYST_CALIB_NOREF	BIT(31)
#define SYST_CALIB_SKEW		BIT(30)

#define SYST_CSR_RUN (SYST_CSR_ENABLE		| \
				SYST_CSR_TICKINT	| \
				SYST_CSR_CLKSOURCE)

/* precompute CYC_PER_TICK at driver init to avoid runtime divisions */
#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
static uint32_t cyc_per_tick;
static ALWAYS_INLINE const uint32_t get_cyc_per_tick(void)
{
	return cyc_per_tick;
}
#else
static ALWAYS_INLINE const uint32_t get_cyc_per_tick(void)
{
	return sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
}
#endif

/* Minimum cycles in the future to try to program.  Note that this is
 * NOT simply "enough cycles to get the counter read and reprogrammed
 * reliably" -- it becomes the minimum value of the RVR register, and
 * thus reflects how much time we can reliably see expire between
 * calls to elapsed() to read the COUNTFLAG bit.  So it needs to be
 * set to be larger than the maximum time the interrupt might be
 * masked.  Choosing a fraction of a tick is probably a good enough
 * default, with an absolute minimum of 1k cyc.
 */
#define MIN_DELAY MAX(1024U, get_cyc_per_tick()/16U)

static struct k_spinlock lock;

static uint32_t last_load;

#ifdef CONFIG_CORTEX_M_SYSTICK_64BIT_CYCLE_COUNTER
typedef uint64_t cycle_t;
#else
typedef uint32_t cycle_t;
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
static uint32_t overflow_cyc;

/* This internal function calculates the amount of HW cycles that have
 * elapsed since the last time the absolute HW cycles counter has been
 * updated. 'cycle_count' may be updated either by the ISR, or when we
 * re-program the SysTick.SYST_RVR register, in sys_clock_set_timeout().
 *
 * Additionally, the function updates the 'overflow_cyc' counter, that
 * holds the amount of elapsed HW cycles due to (possibly) multiple
 * timer wraps (overflows).
 *
 * Prerequisites:
 * - reprogramming of SysTick.SYST_RVR must be clearing the SysTick.CVR
 *   register and the 'overflow_cyc' counter.
 * - ISR must be clearing the 'overflow_cyc' counter.
 * - no more than one counter-wrap has occurred between
 *     - the timer reset or the last time the function was called
 *     - and until the current call of the function is completed.
 * - the function is invoked with interrupts disabled.
 */
static uint32_t elapsed(void)
{
	uint32_t val1 = sys_read32(SYST_CVR);	/* A */
	uint32_t ctrl = sys_read32(SYST_CSR);	/* B */
	uint32_t val2 = sys_read32(SYST_CVR);	/* C */

	/* SysTick behavior: The counter wraps at zero automatically,
	 * setting the COUNTFLAG field of the SYST_CSR register when it
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
	if ((ctrl & SYST_CSR_COUNTFLAG) || (val1 < val2)) {
		overflow_cyc += last_load;

		/* We know there was a wrap, but we might not have
		 * seen it in SYST_CSR, so clear it.
		 */
		sys_read32(SYST_CSR);
	}

	return (last_load - val2) + overflow_cyc;
}

/* sys_clock_isr is calling directly from the platform's vectors table.
 * Also ISR_DIRECT_DECLARE() makes it ready to use and doesn't require any
 * additional hooks via IRQ_CONNECT/IRQ_DIRECT_CONNECT
 */
ISR_DIRECT_DECLARE(sys_clock_isr)
{
	uint32_t dticks;

	k_spinlock_key_t key = k_spin_lock(&lock);
	/* Update overflow_cyc and clear COUNTFLAG by invoking elapsed() */
	elapsed();

	/* Increment the amount of HW cycles elapsed (complete counter
	 * cycles) and announce the progress to the kernel.
	 */
	cycle_count += overflow_cyc;
	overflow_cyc = 0;

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* In TICKLESS mode, the SysTick.SYST_RVR is re-programmed
		 * in sys_clock_set_timeout(), followed by resetting of
		 * the counter (SYST_CVR = 0).
		 *
		 * If a timer wrap occurs right when we re-program SYST_RVR,
		 * the ISR is triggered immediately after
		 * sys_clock_set_timeout() returns; in that case we shall
		 * not increment the cycle_count because the value has been
		 * updated before SYST_RVR re-program.
		 *
		 * We can assess if this is the case by inspecting COUNTFLAG.
		 */
		dticks = (uint32_t)(cycle_count - announced_cycles);
		dticks /= get_cyc_per_tick();
		announced_cycles += dticks * get_cyc_per_tick();
	} else {
		dticks = 1;
	}
	k_spin_unlock(&lock, key);

	sys_clock_announce(dticks);

	ISR_DIRECT_PM();
	return 1;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	const k_ticks_t max_ticks = COUNTER_MAX / get_cyc_per_tick() - 1;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	/* Fast CPUs and a 24 bit counter mean that even idle systems
	 * need to wake up multiple times per second.  If the kernel
	 * allows us to miss tick announcements in idle, then shut off
	 * the counter. (Note: we can assume if idle==true that
	 * interrupts are already disabled)
	 */
	if (ticks == K_TICKS_FOREVER) {
		if (idle) {
			sys_clock_disable();
			return;
		}
		ticks = max_ticks;
	}

	/* passing ticks==1 means "announce the next tick",
	 * ticks value of zero (or even negative) is legal and
	 * treated identically: it simply indicates the
	 * kernel would like the next tick announcement ASAP.
	 */
	ticks = CLAMP(ticks - 1, 0, (const int32_t)max_ticks);

	K_SPINLOCK(&lock) {
		uint32_t delay;
		uint32_t val1, val2;
		uint32_t last_load_ = last_load;

		uint32_t pending = elapsed();

		val1 = sys_read32(SYST_CVR);

		cycle_count += pending;
		overflow_cyc = 0;

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
			const uint32_t max_cycles = max_ticks *
					get_cyc_per_tick();
			/* Desired delay in the future */
			delay = ticks * get_cyc_per_tick();

			/* Round delay up to next tick boundary */
			delay += unannounced;
			delay = DIV_ROUND_UP(delay, get_cyc_per_tick()) *
					get_cyc_per_tick();
			delay -= unannounced;
			delay = MAX(delay, MIN_DELAY);
			if (delay > max_cycles) {
				last_load = max_cycles;
			} else {
				last_load = delay;
			}
		}

		val2 = sys_read32(SYST_CVR);

		sys_write32(last_load - 1, SYST_RVR);
		sys_write32(0, SYST_CVR); /* resets timer to last_load */

		/*
		 * Add elapsed cycles while computing the new load to
		 * cycle_count.
		 *
		 * Note that comparing val1 and val2 is normaly not good enough
		 * to guess if the counter wrapped during this interval.
		 * Indeed if val1 is close to SYST_RVR, then there are little
		 * chances to catch val2 between val1 and SYST_RVR after a wrap.
		 * COUNTFLAG should be checked in addition. But since the load
		 * computation is faster than MIN_DELAY, then we don't need
		 * to worry about this case.
		 */
		if (val1 < val2) {
			cycle_count += (val1 + (last_load_ - val2));
		} else {
			cycle_count += (val1 - val2);
		}
	}
}

uint32_t sys_clock_elapsed(void)
{
	uint32_t ret;

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		k_spinlock_key_t key = k_spin_lock(&lock);
		uint32_t unannounced = cycle_count - announced_cycles;
		uint32_t cyc = elapsed() + unannounced;

		k_spin_unlock(&lock, key);

		ret = cyc / get_cyc_per_tick();
	} else {
		ret = 0;
	}

	return ret;
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
	if (last_load == TIMER_STOPPED) {
		sys_write32(SYST_CSR_RUN, SYST_CSR);
	}
}

void sys_clock_disable(void)
{
	K_SPINLOCK(&lock) {
		sys_write32(0, SYST_CSR);
		last_load = TIMER_STOPPED;
	}
}

static int sys_clock_driver_init(void)
{

	cyc_per_tick = sys_clock_hw_cycles_per_sec() /
			CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	NVIC_SetPriority(SysTick_IRQn, _IRQ_PRIO_OFFSET);
	last_load = get_cyc_per_tick() - 1;
	overflow_cyc = 0;
	sys_write32(last_load, SYST_RVR);
	sys_write32(0, SYST_CVR); /* reset timer to the last_load */
	sys_write32(SYST_CSR_RUN, SYST_CSR);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

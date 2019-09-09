/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <arch/arm/cortex_m/cmsis.h>

void z_arm_exc_exit(void);

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

/* VAL value above which we assume that a subsequent COUNTFLAG
 * overflow seen in CTRL is real and not an artifact of wraparound
 * timing.
 */
#define VAL_ABOUT_TO_WRAP 8

static struct k_spinlock lock;

static u32_t last_load;

static u32_t cycle_count;

static u32_t announced_cycles;

static volatile u32_t overflow_cyc;

static u32_t elapsed(void)
{
	u32_t val, ctrl1, ctrl2;

	/* SysTick is infuriatingly racy.  The counter wraps at zero
	 * automatically, setting a 1 in the COUNTFLAG bit of the CTRL
	 * register when it does.  But reading the control register
	 * automatically resets that bit, so we need to save it for
	 * future calls.  And ordering is critical and race-prone: if
	 * we read CTRL first, then it is possible for VAL to wrap
	 * after that read but before we read VAL and we'll miss the
	 * overflow.  If we read VAL first, then it can wrap after we
	 * read it and we'll see an "extra" overflow in CTRL.  And we
	 * want to handle multiple overflows, so we effectively must
	 * read CTRL first otherwise there will be no way to detect
	 * the double-overflow if called at the end of a cycle.  There
	 * is no safe algorithm here, so we split the difference by
	 * reading CTRL twice, suppressing the second overflow bit if
	 * VAL was "about to overflow".
	 */
	ctrl1 = SysTick->CTRL;
	val = SysTick->VAL & COUNTER_MAX;
	ctrl2 = SysTick->CTRL;

	overflow_cyc += (ctrl1 & SysTick_CTRL_COUNTFLAG_Msk) ? last_load : 0;
	if (val > VAL_ABOUT_TO_WRAP) {
		int wrap = ctrl2 & SysTick_CTRL_COUNTFLAG_Msk;

		overflow_cyc += (wrap != 0) ? last_load : 0;
	}

	return (last_load - val) + overflow_cyc;
}

/* Callout out of platform assembly, not hooked via IRQ_CONNECT... */
void z_clock_isr(void *arg)
{
	ARG_UNUSED(arg);
	u32_t dticks;

	cycle_count += last_load;
	dticks = (cycle_count - announced_cycles) / CYC_PER_TICK;
	announced_cycles += dticks * CYC_PER_TICK;

	overflow_cyc = SysTick->CTRL; /* Reset overflow flag */
	overflow_cyc = 0U;

	z_clock_announce(TICKLESS ? dticks : 1);
	z_arm_exc_exit();
}

int z_clock_driver_init(struct device *device)
{
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

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	/* Fast CPUs and a 24 bit counter mean that even idle systems
	 * need to wake up multiple times per second.  If the kernel
	 * allows us to miss tick announcements in idle, then shut off
	 * the counter. (Note: we can assume if idle==true that
	 * interrupts are already disabled)
	 */
	if (IS_ENABLED(CONFIG_TICKLESS_IDLE) && idle && ticks == K_FOREVER) {
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		last_load = TIMER_STOPPED;
		return;
	}

#if defined(CONFIG_TICKLESS_KERNEL)
	u32_t delay;

	ticks = MIN(MAX_TICKS, MAX(ticks - 1, 0));

	/* Desired delay in the future */
	delay = (ticks == 0) ? MIN_DELAY : ticks * CYC_PER_TICK;

	k_spinlock_key_t key = k_spin_lock(&lock);

	cycle_count += elapsed();

	/* Round delay up to next tick boundary */
	delay = delay + (cycle_count - announced_cycles);
	delay = ((delay + CYC_PER_TICK - 1) / CYC_PER_TICK) * CYC_PER_TICK;
	last_load = delay - (cycle_count - announced_cycles);

	overflow_cyc = 0U;
	SysTick->LOAD = last_load - 1;
	SysTick->VAL = 0; /* resets timer to last_load */

	k_spin_unlock(&lock, key);
#endif
}

u32_t z_clock_elapsed(void)
{
	if (!TICKLESS) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t cyc = elapsed() + cycle_count - announced_cycles;

	k_spin_unlock(&lock, key);
	return cyc / CYC_PER_TICK;
}

u32_t z_timer_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = elapsed() + cycle_count;

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

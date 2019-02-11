/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <arch/arm/cortex_m/cmsis.h>

void _ExcExit(void);

/* Minimum cycles in the future to try to program. */
#define MIN_DELAY 512

#define COUNTER_MAX 0x00ffffff
#define TIMER_STOPPED 0xff000000

#define CYC_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((COUNTER_MAX / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

#define TICKLESS (IS_ENABLED(CONFIG_TICKLESS_KERNEL) &&			\
		  !IS_ENABLED(CONFIG_QEMU_TICKLESS_WORKAROUND))

static struct k_spinlock lock;

static u32_t last_load;

static u32_t cycle_count;

static u32_t announced_cycles;

static volatile u32_t ctrl_cache; /* overflow bit clears on read! */

static u32_t elapsed(void)
{
	u32_t val, ov;

	do {
		val = SysTick->VAL & COUNTER_MAX;
		ctrl_cache |= SysTick->CTRL;
	} while (SysTick->VAL > val);

	ov = (ctrl_cache & SysTick_CTRL_COUNTFLAG_Msk) ? last_load : 0;
	return (last_load - val) + ov;
}

/* Callout out of platform assembly, not hooked via IRQ_CONNECT... */
void z_clock_isr(void *arg)
{
	ARG_UNUSED(arg);
	u32_t dticks;

	cycle_count += last_load;
	dticks = (cycle_count - announced_cycles) / CYC_PER_TICK;
	announced_cycles += dticks * CYC_PER_TICK;

	ctrl_cache = SysTick->CTRL; /* Reset overflow flag */
	ctrl_cache = 0U;

	z_clock_announce(TICKLESS ? dticks : 1);
	_ExcExit();
}

int z_clock_driver_init(struct device *device)
{
	NVIC_SetPriority(SysTick_IRQn, _IRQ_PRIO_OFFSET);
	last_load = CYC_PER_TICK;
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

#if defined(CONFIG_TICKLESS_KERNEL) && !defined(CONFIG_QEMU_TICKLESS_WORKAROUND)
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

	SysTick->LOAD = last_load;
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

u32_t _timer_cycle_get_32(void)
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

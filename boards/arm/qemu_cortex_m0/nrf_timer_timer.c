/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <hal/nrf_timer.h>
#include <spinlock.h>

#define TIMER NRF_TIMER0

#define COUNTER_MAX 0xffffffff
#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((COUNTER_MAX - CYC_PER_TICK) / CYC_PER_TICK)

static struct k_spinlock lock;

static u32_t last_count;

static u32_t counter_sub(u32_t a, u32_t b)
{
	return (a - b) & COUNTER_MAX;
}

static void set_comparator(u32_t cyc)
{
	nrf_timer_cc_write(TIMER, 0, cyc & COUNTER_MAX);
}

static u32_t counter(void)
{
	nrf_timer_task_trigger(TIMER, nrf_timer_capture_task_get(1));

	return nrf_timer_cc_read(TIMER, 1);
}

void timer0_nrf_isr(void *arg)
{
	ARG_UNUSED(arg);
	TIMER->EVENTS_COMPARE[0] = 0;

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t t = counter();
	u32_t dticks = counter_sub(t, last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		u32_t next = last_count + CYC_PER_TICK;

		/* As below: we're guaranteed to get an interrupt as
		 * long as it's set two or more cycles in the future
		 */
		if (counter_sub(next, t) < 3) {
			next += CYC_PER_TICK;
		}
		set_comparator(next);
	}

	k_spin_unlock(&lock, key);
	z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

int z_clock_driver_init(struct device *device)
{
	struct device *clock;

	ARG_UNUSED(device);

	clock = device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_16M");
	if (!clock) {
		return -1;
	}

	/* turn on clock in blocking mode. */
	clock_control_on(clock, (void *)1);

	nrf_timer_frequency_set(TIMER, NRF_TIMER_FREQ_1MHz);
	nrf_timer_bit_width_set(TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_cc_write(TIMER, 0, CYC_PER_TICK);
	nrf_timer_int_enable(TIMER, TIMER_INTENSET_COMPARE0_Msk);

	/* Clear the event flag and possible pending interrupt */
	nrf_timer_event_clear(TIMER, NRF_TIMER_EVENT_COMPARE0);
	NVIC_ClearPendingIRQ(TIMER0_IRQn);

	IRQ_CONNECT(TIMER0_IRQn, 1, timer0_nrf_isr, 0, 0);
	irq_enable(TIMER0_IRQn);

	nrf_timer_task_trigger(TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_task_trigger(TIMER, NRF_TIMER_TASK_START);

	if (!IS_ENABLED(TICKLESS_KERNEL)) {
		set_comparator(counter() + CYC_PER_TICK);
	}

	return 0;
}

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#ifdef CONFIG_TICKLESS_KERNEL
	ticks = (ticks == K_FOREVER) ? MAX_TICKS : ticks;
	ticks = MAX(MIN(ticks - 1, (s32_t)MAX_TICKS), 0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t cyc, dt, t = counter();
	bool zli_fixup = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS);

	/* Round up to next tick boundary */
	cyc = ticks * CYC_PER_TICK + 1 + counter_sub(t, last_count);
	cyc += (CYC_PER_TICK - 1);
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;
	cyc += last_count;

	if (counter_sub(cyc, t) > 2) {
		set_comparator(cyc);
	} else {
		set_comparator(cyc);
		dt = counter_sub(cyc, counter());
		if (dt == 0 || dt > 0x7fffff) {
			/* Missed it! */
			NVIC_SetPendingIRQ(TIMER0_IRQn);
			if (IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)) {
				zli_fixup = false;
			}
		} else if (dt == 1) {
			/* Too soon, interrupt won't arrive. */
			set_comparator(cyc + 2);
		}
		/* Otherwise it was two cycles out, we're fine */
	}

#ifdef CONFIG_ZERO_LATENCY_IRQS
	/* Failsafe.  ZLIs can preempt us even though interrupts are
	 * masked, blowing up the sensitive timing above.  If the
	 * feature is enabled and we haven't recorded the presence of
	 * a pending interrupt then we need a final check (in a loop!
	 * because this too can be interrupted) to confirm that the
	 * comparator is still in the future.  Don't bother being
	 * fancy with cycle counting here, just set an interrupt
	 * "soon" that we know will get the timer back to a known
	 * state.  This handles (via some hairy modular expressions)
	 * the wraparound cases where we are preempted for as much as
	 * half the counter space.
	 */
	if (zli_fixup && counter_sub(cyc, counter()) <= 0x7fffff) {
		while (counter_sub(cyc, counter() + 2) > 0x7fffff) {
			cyc = counter() + 3;
			set_comparator(cyc);
		}
	}
#endif

	k_spin_unlock(&lock, key);
#endif /* CONFIG_TICKLESS_KERNEL */
}

u32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = counter_sub(counter(), last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

u32_t z_timer_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = counter_sub(counter(), last_count) + last_count;

	k_spin_unlock(&lock, key);
	return ret;
}

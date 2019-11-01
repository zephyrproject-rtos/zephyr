/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <hal/nrf_rtc.h>
#include <spinlock.h>

#define RTC NRF_RTC1

#define COUNTER_MAX 0x00ffffff
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
	nrf_rtc_cc_set(RTC, 0, cyc & COUNTER_MAX);
}

static u32_t counter(void)
{
	return nrf_rtc_counter_get(RTC);
}

/* Note: this function has public linkage, and MUST have this
 * particular name.  The platform architecture itself doesn't care,
 * but there is a test (tests/arch/arm_irq_vector_table) that needs
 * to find it to it can set it in a custom vector table.  Should
 * probably better abstract that at some point (e.g. query and reset
 * it by pointer at runtime, maybe?) so we don't have this leaky
 * symbol.
 */
void rtc1_nrf_isr(void *arg)
{
	ARG_UNUSED(arg);
	RTC->EVENTS_COMPARE[0] = 0;

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

	clock = device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_32K");
	if (!clock) {
		return -1;
	}

	clock_control_on(clock, NULL);

	/* TODO: replace with counter driver to access RTC */
	nrf_rtc_prescaler_set(RTC, 0);
	nrf_rtc_cc_set(RTC, 0, CYC_PER_TICK);
	nrf_rtc_int_enable(RTC, RTC_INTENSET_COMPARE0_Msk);

	/* Clear the event flag and possible pending interrupt */
	nrf_rtc_event_clear(RTC, NRF_RTC_EVENT_COMPARE_0);
	NVIC_ClearPendingIRQ(RTC1_IRQn);

	IRQ_CONNECT(RTC1_IRQn, 1, rtc1_nrf_isr, 0, 0);
	irq_enable(RTC1_IRQn);

	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_CLEAR);
	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_START);

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

	/* Per NRF docs, the RTC is guaranteed to trigger a compare
	 * event if the comparator value to be set is at least two
	 * cycles later than the current value of the counter.  So if
	 * we're three or more cycles out, we can set it blindly.  If
	 * not, check the time again immediately after setting: it's
	 * possible we "just missed it" and can flag an immediate
	 * interrupt.  Or it could be exactly two cycles out, which
	 * will have worked.  Otherwise, there's no way to get an
	 * interrupt at the right time and we have to slip the event
	 * by one clock cycle (or we could spin, but this is a slow
	 * clock and spinning for a whole cycle can be thousands of
	 * instructions!)
	 *
	 * You might ask: why not set the comparator first and then
	 * check the timer synchronously to see if we missed it, which
	 * would avoid the need for a slipped cycle.  That doesn't
	 * work, the states overlap inside the counter hardware.  It's
	 * possible to set a comparator value of "N", issue a DSB
	 * instruction to flush the pipeline, and then immediately
	 * read a counter value of "N-1" (i.e. the comparator is still
	 * in the future), and yet still not receive an interrupt at
	 * least on nRF52.  Some experimentation on nrf52840 shows
	 * that you need to be early by about 400 processor cycles
	 * (about 1/5th of a RTC cycle) in order to reliably get the
	 * interrupt.  The docs say two cycles, they mean two cycles.
	 */
	if (counter_sub(cyc, t) > 2) {
		set_comparator(cyc);
	} else {
		set_comparator(cyc);
		dt = counter_sub(cyc, counter());
		if (dt == 0 || dt > 0x7fffff) {
			/* Missed it! */
			NVIC_SetPendingIRQ(RTC1_IRQn);
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

/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <system_timer.h>
#include <sys_clock.h>
#include <nrf_rtc.h>
#include <spinlock.h>

#define RTC NRF_RTC1

/*
 * Compare values must be set to at least 2 greater than the current
 * counter value to ensure that the compare fires.  Compare values are
 * generally determined by reading the counter, then performing some
 * calculations to convert a relative delay to an absolute delay.
 * Assume that the counter will not increment more than twice during
 * these calculations, allowing for a final check that can replace a
 * too-low compare with a value that will guarantee fire.
 */
#define MIN_DELAY 4

#define CYC_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#if CYC_PER_TICK < MIN_DELAY
#error Cycles per tick is too small
#endif

#define COUNTER_MAX 0x00ffffffU
#define MAX_TICKS ((COUNTER_MAX - MIN_DELAY) / CYC_PER_TICK)
#define MAX_DELAY (MAX_TICKS * CYC_PER_TICK)

static u32_t last_count;

static inline u32_t counter_sub(u32_t a, u32_t b)
{
	return (a - b) & COUNTER_MAX;
}

static inline void set_comparator(u32_t cyc)
{
	nrf_rtc_cc_set(RTC, 0, cyc);
}

static inline u32_t counter(void)
{
	return nrf_rtc_counter_get(RTC);
}

/* Note: this function has public linkage, and MUST have this
 * particular name.  The platform architecture itself doesn't care,
 * but there is a test (tests/kernel/arm_irq_vector_table) that needs
 * to find it to it can set it in a custom vector table.  Should
 * probably better abstract that at some point (e.g. query and reset
 * it by pointer at runtime, maybe?) so we don't have this leaky
 * symbol.
 */
void rtc1_nrf_isr(void *arg)
{
	ARG_UNUSED(arg);
	RTC->EVENTS_COMPARE[0] = 0;

	u32_t key = irq_lock();
	u32_t t = counter();
	u32_t dticks = counter_sub(t, last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		u32_t next = last_count + CYC_PER_TICK;

		if (counter_sub(next, t) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_comparator(next);
	}

	irq_unlock(key);
	z_clock_announce(dticks);
}

int z_clock_driver_init(struct device *device)
{
	struct device *clock;

	ARG_UNUSED(device);

	clock = device_get_binding(CONFIG_CLOCK_CONTROL_NRF_K32SRC_DRV_NAME);
	if (!clock) {
		return -1;
	}

	clock_control_on(clock, (void *)CLOCK_CONTROL_NRF_K32SRC);

	/* TODO: replace with counter driver to access RTC */
	nrf_rtc_prescaler_set(RTC, 0);
	nrf_rtc_cc_set(RTC, 0, CYC_PER_TICK);
	nrf_rtc_event_enable(RTC, RTC_EVTENSET_COMPARE0_Msk);
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
	ticks = max(min(ticks - 1, (s32_t)MAX_TICKS), 0);

	/*
	 * Get the requested delay in tick-aligned cycles.  Increase
	 * by one tick to round up so we don't timeout early due to
	 * cycles elapsed since the last tick.  Cap at the maximum
	 * tick-aligned delta.
	 */
	u32_t cyc = min((1 + ticks) * CYC_PER_TICK, MAX_DELAY);

	u32_t key = irq_lock();
	u32_t d = counter_sub(counter(), last_count);

	/*
	 * We've already accounted for anything less than a full tick,
	 * and assumed we meet the minimum delay for the tick.  If
	 * that's not true, we have to adjust, which may involve a
	 * rare and expensive integer division.
	 */
	if (d > (CYC_PER_TICK - MIN_DELAY)) {
		if (d >= CYC_PER_TICK) {
			/*
			 * We're late by at least one tick.  Adjust
			 * the compare offset for the missed ones, and
			 * reduce d to be the portion since the last
			 * (unseen) tick.
			 */
			u32_t missed_ticks = d / CYC_PER_TICK;
			u32_t missed_cycles = missed_ticks * CYC_PER_TICK;
			cyc += missed_cycles;
			d -= missed_cycles;
		}
		if (d > (CYC_PER_TICK - MIN_DELAY)) {
			/*
			 * We're (now) within the tick, but too close
			 * to meet the minimum delay required to
			 * guarantee compare firing.  Step up to the
			 * next tick.
			 */
			cyc += CYC_PER_TICK;
		}
		if (cyc > MAX_DELAY) {
			/* Don't adjust beyond the counter range. */
			cyc = MAX_DELAY;
		}
	}
	set_comparator(last_count + cyc);

	irq_unlock(key);
#endif
}

u32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	u32_t key = irq_lock();
	u32_t ret = counter_sub(counter(), last_count) / CYC_PER_TICK;

	irq_unlock(key);
	return ret;
}

u32_t _timer_cycle_get_32(void)
{
	u32_t key = irq_lock();
	u32_t ret = counter_sub(counter(), last_count) + last_count;

	irq_unlock(key);
	return ret;
}

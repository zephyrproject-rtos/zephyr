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
#define RTC_IRQn NRFX_IRQ_NUMBER_GET(RTC)

#define COUNTER_SPAN BIT(24)
#define COUNTER_MAX (COUNTER_SPAN - 1U)
#define COUNTER_HALF_SPAN (COUNTER_SPAN / 2U)
#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((COUNTER_HALF_SPAN - CYC_PER_TICK) / CYC_PER_TICK)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

static struct k_spinlock lock;

static uint32_t last_count;

static uint32_t counter_sub(uint32_t a, uint32_t b)
{
	return (a - b) & COUNTER_MAX;
}

static void set_comparator(uint32_t cyc)
{
	nrf_rtc_cc_set(RTC, 0, cyc & COUNTER_MAX);
}

static uint32_t get_comparator(void)
{
	return nrf_rtc_cc_get(RTC, 0);
}

static void event_clear(void)
{
	nrf_rtc_event_clear(RTC, NRF_RTC_EVENT_COMPARE_0);
}

static void event_enable(void)
{
	nrf_rtc_event_enable(RTC, NRF_RTC_INT_COMPARE0_MASK);
}

static void int_disable(void)
{
	nrf_rtc_int_disable(RTC, NRF_RTC_INT_COMPARE0_MASK);
}

static void int_enable(void)
{
	nrf_rtc_int_enable(RTC, NRF_RTC_INT_COMPARE0_MASK);
}

static uint32_t counter(void)
{
	return nrf_rtc_counter_get(RTC);
}

/* Function ensures that previous CC value will not set event */
static void prevent_false_prev_evt(void)
{
	uint32_t now = counter();
	uint32_t prev_val;

	/* First take care of a risk of an event coming from CC being set to the
	 * next cycle.
	 * Reconfigure CC to the future. If CC was set to next cycle we need to
	 * wait for up to 15 us (half of 32 kHz interval) and clean a potential
	 * event. After that there is no risk of unwanted event.
	 */
	prev_val = get_comparator();
	event_clear();
	set_comparator(now);
	event_enable();

	if (counter_sub(prev_val, now) == 1) {
		k_busy_wait(15);
		event_clear();
	}

	/* Clear interrupt that may have fired as we were setting the
	 * comparator.
	 */
	NVIC_ClearPendingIRQ(RTC_IRQn);
}

/* If alarm is next RTC cycle from now, function attempts to adjust. If
 * counter progresses during that time it means that 1 cycle elapsed and
 * interrupt is set pending.
 */
static void handle_next_cycle_case(uint32_t t)
{
	set_comparator(t + 2);
	while (t != counter()) {
		/* Already expired, time elapsed but event might not be
		 * generated. Trigger interrupt.
		 */
		t = counter();
		set_comparator(t + 2);
	}
}

/* Function safely sets absolute alarm. It assumes that provided value is
 * less than MAX_CYCLES from now. It detects late setting and also handles
 * +1 cycle case.
 */
static void set_absolute_alarm(uint32_t abs_val)
{
	uint32_t diff;
	uint32_t t = counter();

	diff = counter_sub(abs_val, t);
	if (diff == 1) {
		handle_next_cycle_case(t);
		return;
	}

	set_comparator(abs_val);
	t = counter();
	/* A little trick, subtract 2 to force now and now + 1 case fall into
	 * negative (> MAX_CYCLES). Diff 0 means two cycles from now.
	 */
	diff = counter_sub(abs_val - 2, t);
	if (diff > MAX_CYCLES) {
		/* Already expired, set for subsequent cycle. */
		/* It is possible that setting CC was interrupted and CC might
		 * be set to COUNTER+1 value which will not generate an event.
		 * In that case, special handling is performed (attempt to set
		 * CC to COUNTER+2).
		 */
		handle_next_cycle_case(t);
	}
}

/* Sets relative alarm from any context. Function is lockless. It only
 * blocks RTC interrupt.
 */
static void set_protected_absolute_alarm(uint32_t cycles)
{
	int_disable();

	prevent_false_prev_evt();

	set_absolute_alarm(cycles);

	int_enable();
}

/* Note: this function has public linkage, and MUST have this
 * particular name.  The platform architecture itself doesn't care,
 * but there is a test (tests/arch/arm_irq_vector_table) that needs
 * to find it to it can set it in a custom vector table.  Should
 * probably better abstract that at some point (e.g. query and reset
 * it by pointer at runtime, maybe?) so we don't have this leaky
 * symbol.
 */
void rtc_nrf_isr(void *arg)
{
	ARG_UNUSED(arg);
	event_clear();

	uint32_t t = get_comparator();
	uint32_t dticks = counter_sub(t, last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* protection is not needed because we are in the RTC interrupt
		 * so it won't get preempted by the interrupt.
		 */
		set_absolute_alarm(last_count + CYC_PER_TICK);
	}

	z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : (dticks > 0));
}

int z_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	/* TODO: replace with counter driver to access RTC */
	nrf_rtc_prescaler_set(RTC, 0);
	event_clear();
	NVIC_ClearPendingIRQ(RTC_IRQn);
	int_enable();

	IRQ_CONNECT(RTC_IRQn, 1, rtc_nrf_isr, 0, 0);
	irq_enable(RTC_IRQn);

	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_CLEAR);
	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_START);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		set_comparator(counter() + CYC_PER_TICK);
	}

	z_nrf_clock_control_lf_on(NRF_LFCLK_START_MODE_NOWAIT);

	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
	uint32_t cyc;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = MAX(MIN(ticks - 1, (int32_t)MAX_TICKS), 0);

	uint32_t unannounced = counter_sub(counter(), last_count);

	/* If we haven't announced for more than half the 24-bit wrap
	 * duration, then force an announce to avoid loss of a wrap
	 * event.  This can happen if new timeouts keep being set
	 * before the existing one triggers the interrupt.
	 */
	if (unannounced >= COUNTER_HALF_SPAN) {
		ticks = 0;
	}

	/* Get the cycles from last_count to the tick boundary after
	 * the requested ticks have passed starting now.
	 */
	cyc = ticks * CYC_PER_TICK + 1 + unannounced;
	cyc += (CYC_PER_TICK - 1);
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;

	/* Due to elapsed time the calculation above might produce a
	 * duration that laps the counter.  Don't let it.
	 */
	if (cyc > MAX_CYCLES) {
		cyc = MAX_CYCLES;
	}

	cyc += last_count;
	set_protected_absolute_alarm(cyc);
}

uint32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = counter_sub(counter(), last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t z_timer_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = counter_sub(counter(), last_count) + last_count;

	k_spin_unlock(&lock, key);
	return ret;
}

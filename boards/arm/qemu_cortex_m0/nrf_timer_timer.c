/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <hal/nrf_timer.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>

#define TIMER NRF_TIMER0

#define COUNTER_MAX 0xFFFFFFFFUL
#define COUNTER_HALF_SPAN 0x80000000UL
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
	nrf_timer_cc_set(TIMER, NRF_TIMER_CC_CHANNEL0, cyc & COUNTER_MAX);
}

static uint32_t get_comparator(void)
{
	return nrf_timer_cc_get(TIMER, NRF_TIMER_CC_CHANNEL0);
}

static void event_clear(void)
{
	nrf_timer_event_clear(TIMER, NRF_TIMER_EVENT_COMPARE0);
}

static void int_disable(void)
{
	nrf_timer_int_disable(TIMER, NRF_TIMER_INT_COMPARE0_MASK);
}

static void int_enable(void)
{
	nrf_timer_int_enable(TIMER, NRF_TIMER_INT_COMPARE0_MASK);
}

static uint32_t counter(void)
{
	nrf_timer_task_trigger(TIMER,
		nrf_timer_capture_task_get(NRF_TIMER_CC_CHANNEL1));

	return nrf_timer_cc_get(TIMER, NRF_TIMER_CC_CHANNEL1);
}

/* Function ensures that previous CC value will not set event */
static void prevent_false_prev_evt(void)
{
	uint32_t now = counter();
	uint32_t prev_val;

	/* First take care of a risk of an event coming from CC being set to
	 * next tick. Reconfigure CC to future (now tick is the furthest
	 * future). If CC was set to next tick we need to wait for up to 0.5us
	 * (half of 1M tick) and clean potential event. After that time there
	 * is no risk of unwanted event.
	 */
	prev_val = get_comparator();
	event_clear();
	set_comparator(now);

	if (counter_sub(prev_val, now) == 1) {
		k_busy_wait(1);
		event_clear();
	}

	/* Clear interrupt that may have fired as we were setting the
	 * comparator.
	 */
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
}

/* If settings is next tick from now, function attempts to set next tick. If
 * counter progresses during that time it means that 1 tick elapsed and
 * interrupt is set pending.
 */
static void handle_next_tick_case(uint32_t t)
{
	set_comparator(t + 2U);
	while (t != counter()) {
		/* already expired, tick elapsed but event might not be
		 * generated. Trigger interrupt.
		 */
		t = counter();
		set_comparator(t + 2U);
	}
}

/* Function safely sets absolute alarm. It assumes that provided value is
 * less than MAX_TICKS from now. It detects late setting and also handles
 * +1 tick case.
 */
static void set_absolute_ticks(uint32_t abs_val)
{
	uint32_t diff;
	uint32_t t = counter();

	diff = counter_sub(abs_val, t);
	if (diff == 1U) {
		handle_next_tick_case(t);
		return;
	}

	set_comparator(abs_val);
}

/* Sets relative ticks alarm from any context. Function is lockless. It only
 * blocks TIMER interrupt.
 */
static void set_protected_absolute_ticks(uint32_t ticks)
{
	int_disable();

	prevent_false_prev_evt();

	set_absolute_ticks(ticks);

	int_enable();
}

void timer0_nrf_isr(void *arg)
{
	ARG_UNUSED(arg);
	event_clear();


	uint32_t t = get_comparator();
	uint32_t dticks = counter_sub(t, last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* protection is not needed because we are in the TIMER interrupt
		 * so it won't get preempted by the interrupt.
		 */
		set_absolute_ticks(last_count + CYC_PER_TICK);
	}

	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : (dticks > 0));
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
	uint32_t cyc;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

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
	set_protected_absolute_ticks(cyc);

	/* FIXME - QEMU requires clearing the events when setting the comparator,
	 * but the TIMER peripheral HW does not need this. Remove when fixed in
	 * QEMU.
	 */
	event_clear();
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = counter_sub(counter(), last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = counter_sub(counter(), last_count) + last_count;

	k_spin_unlock(&lock, key);
	return ret;
}

static int sys_clock_driver_init(void)
{

	/* FIXME switch to 1 MHz once this is fixed in QEMU */
	nrf_timer_prescaler_set(TIMER, NRF_TIMER_FREQ_2MHz);
	nrf_timer_bit_width_set(TIMER, NRF_TIMER_BIT_WIDTH_32);

	IRQ_CONNECT(TIMER0_IRQn, 1, timer0_nrf_isr, 0, 0);
	irq_enable(TIMER0_IRQn);

	nrf_timer_task_trigger(TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_task_trigger(TIMER, NRF_TIMER_TASK_START);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		set_comparator(counter() + CYC_PER_TICK);
	}

	event_clear();
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	int_enable();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

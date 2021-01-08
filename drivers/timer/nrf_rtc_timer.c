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
#include <drivers/timer/nrf_rtc_timer.h>
#include <sys_clock.h>
#include <hal/nrf_rtc.h>
#include <spinlock.h>


#define EXT_CHAN_COUNT CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT
#define CHAN_COUNT (EXT_CHAN_COUNT + 1)

#define RTC NRF_RTC1
#define RTC_IRQn NRFX_IRQ_NUMBER_GET(RTC)
#define RTC_LABEL rtc1
#define RTC_CH_COUNT RTC1_CC_NUM

BUILD_ASSERT(CHAN_COUNT <= RTC_CH_COUNT, "Not enough compare channels");

#define COUNTER_SPAN BIT(24)
#define COUNTER_MAX (COUNTER_SPAN - 1U)
#define COUNTER_HALF_SPAN (COUNTER_SPAN / 2U)
#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((COUNTER_HALF_SPAN - CYC_PER_TICK) / CYC_PER_TICK)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

static struct k_spinlock lock;

static uint32_t last_count;

struct z_nrf_rtc_timer_chan_data {
	z_nrf_rtc_timer_compare_handler_t callback;
	void *user_context;
};

static struct z_nrf_rtc_timer_chan_data cc_data[CHAN_COUNT];
static atomic_t int_mask;
static atomic_t alloc_mask;

static uint32_t counter_sub(uint32_t a, uint32_t b)
{
	return (a - b) & COUNTER_MAX;
}

static void set_comparator(uint32_t chan, uint32_t cyc)
{
	nrf_rtc_cc_set(RTC, chan, cyc & COUNTER_MAX);
}

static uint32_t get_comparator(uint32_t chan)
{
	return nrf_rtc_cc_get(RTC, chan);
}

static void event_clear(uint32_t chan)
{
	nrf_rtc_event_clear(RTC, RTC_CHANNEL_EVENT_ADDR(chan));
}

static void event_enable(uint32_t chan)
{
	nrf_rtc_event_enable(RTC, RTC_CHANNEL_INT_MASK(chan));
}

static void event_disable(uint32_t chan)
{
	nrf_rtc_event_disable(RTC, RTC_CHANNEL_INT_MASK(chan));
}

static uint32_t counter(void)
{
	return nrf_rtc_counter_get(RTC);
}

uint32_t z_nrf_rtc_timer_read(void)
{
	return nrf_rtc_counter_get(RTC);
}

uint32_t z_nrf_rtc_timer_compare_evt_address_get(uint32_t chan)
{
	__ASSERT_NO_MSG(chan < CHAN_COUNT);
	return nrf_rtc_event_address_get(RTC, nrf_rtc_compare_event_get(chan));
}

bool z_nrf_rtc_timer_compare_int_lock(uint32_t chan)
{
	__ASSERT_NO_MSG(chan && chan < CHAN_COUNT);

	atomic_val_t prev = atomic_and(&int_mask, ~BIT(chan));

	nrf_rtc_int_disable(RTC, RTC_CHANNEL_INT_MASK(chan));

	return prev & BIT(chan);
}

void z_nrf_rtc_timer_compare_int_unlock(uint32_t chan, bool key)
{
	__ASSERT_NO_MSG(chan && chan < CHAN_COUNT);

	if (key) {
		atomic_or(&int_mask, BIT(chan));
		nrf_rtc_int_enable(RTC, RTC_CHANNEL_INT_MASK(chan));
	}
}

uint32_t z_nrf_rtc_timer_compare_read(uint32_t chan)
{
	__ASSERT_NO_MSG(chan < CHAN_COUNT);

	return nrf_rtc_cc_get(RTC, chan);
}

int z_nrf_rtc_timer_get_ticks(k_timeout_t t)
{
	uint32_t curr_count;
	int64_t curr_tick;
	int64_t result;
	int64_t abs_ticks;

	do {
		curr_count = counter();
		curr_tick = z_tick_get();
	} while (curr_count != counter());

	abs_ticks = Z_TICK_ABS(t.ticks);
	if (abs_ticks < 0) {
		/* relative timeout */
		return (t.ticks > COUNTER_HALF_SPAN) ?
			-EINVAL : ((curr_count + t.ticks) & COUNTER_MAX);
	}

	/* absolute timeout */
	result = abs_ticks - curr_tick;

	if ((result > COUNTER_HALF_SPAN) ||
	    (result < -(int64_t)COUNTER_HALF_SPAN)) {
		return -EINVAL;
	}

	return (curr_count + result) & COUNTER_MAX;
}

/* Function safely sets absolute alarm. It assumes that provided value is
 * less than COUNTER_HALF_SPAN from now. It detects late setting and also
 * handle +1 cycle case.
 */
static void set_absolute_alarm(uint32_t chan, uint32_t abs_val)
{
	uint32_t now;
	uint32_t now2;
	uint32_t cc_val = abs_val & COUNTER_MAX;
	uint32_t prev_cc = get_comparator(chan);

	do {
		now = counter();

		/* Handle case when previous event may generate an event.
		 * It is handled by setting CC to now (far in the future),
		 * in case previous event was set for next tick wait for half
		 * LF tick and clear event that may have been generated.
		 */
		set_comparator(chan, now);
		if (counter_sub(prev_cc, now) == 1) {
			/* It should wait for half of RTC tick 15.26us. As
			 * busy wait runs from different clock source thus
			 * wait longer to cover for discrepancy.
			 */
			k_busy_wait(19);
		}


		/* If requested cc_val is in the past or next tick, set to 2
		 * ticks from now. RTC may not generate event if CC is set for
		 * 1 tick from now.
		 */
		if (counter_sub(cc_val, now + 2) > COUNTER_HALF_SPAN) {
			cc_val = now + 2;
		}

		event_clear(chan);
		event_enable(chan);
		set_comparator(chan, cc_val);
		now2 = counter();
		prev_cc = cc_val;
		/* Rerun the algorithm if counter progressed during execution
		 * and cc_val is in the past or one tick from now. In such
		 * scenario, it is possible that event will not be generated.
		 * Reruning the algorithm will delay the alarm but ensure that
		 * event will be generated at the moment indicated by value in
		 * CC register.
		 */
	} while ((now2 != now) &&
		 (counter_sub(cc_val, now2 + 2) > COUNTER_HALF_SPAN));
}

static void compare_set(uint32_t chan, uint32_t cc_value,
			z_nrf_rtc_timer_compare_handler_t handler,
			void *user_data)
{
	cc_data[chan].callback = handler;
	cc_data[chan].user_context = user_data;

	set_absolute_alarm(chan, cc_value);
}

void z_nrf_rtc_timer_compare_set(uint32_t chan, uint32_t cc_value,
			      z_nrf_rtc_timer_compare_handler_t handler,
			      void *user_data)
{
	__ASSERT_NO_MSG(chan && chan < CHAN_COUNT);

	bool key = z_nrf_rtc_timer_compare_int_lock(chan);

	compare_set(chan, cc_value, handler, user_data);

	z_nrf_rtc_timer_compare_int_unlock(chan, key);
}

static void sys_clock_timeout_handler(uint32_t chan,
				      uint32_t cc_value,
				      void *user_data)
{
	uint32_t dticks = counter_sub(cc_value, last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* protection is not needed because we are in the RTC interrupt
		 * so it won't get preempted by the interrupt.
		 */
		compare_set(chan, last_count + CYC_PER_TICK,
					  sys_clock_timeout_handler, NULL);
	}

	z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ?
						dticks : (dticks > 0));
}

/* Note: this function has public linkage, and MUST have this
 * particular name.  The platform architecture itself doesn't care,
 * but there is a test (tests/arch/arm_irq_vector_table) that needs
 * to find it to it can set it in a custom vector table.  Should
 * probably better abstract that at some point (e.g. query and reset
 * it by pointer at runtime, maybe?) so we don't have this leaky
 * symbol.
 */
void rtc_nrf_isr(const void *arg)
{
	ARG_UNUSED(arg);

	for (uint32_t chan = 0; chan < CHAN_COUNT; chan++) {
		if (nrf_rtc_int_enable_check(RTC, RTC_CHANNEL_INT_MASK(chan)) &&
		    nrf_rtc_event_check(RTC, RTC_CHANNEL_EVENT_ADDR(chan))) {
			uint32_t cc_val;
			z_nrf_rtc_timer_compare_handler_t handler;

			event_clear(chan);
			event_disable(chan);
			cc_val = get_comparator(chan);
			handler = cc_data[chan].callback;
			cc_data[chan].callback = NULL;
			if (handler) {
				handler(chan, cc_val,
					cc_data[chan].user_context);
			}
		}
	}
}

int z_nrf_rtc_timer_chan_alloc(void)
{
	int chan;
	atomic_val_t prev;
	do {
		chan = alloc_mask ? 31 - __builtin_clz(alloc_mask) : -1;
		if (chan < 0) {
			return -ENOMEM;
		}
		prev = atomic_and(&alloc_mask, ~BIT(chan));
	} while (!(prev & BIT(chan)));

	return chan;
}

void z_nrf_rtc_timer_chan_free(uint32_t chan)
{
	__ASSERT_NO_MSG(chan && chan < CHAN_COUNT);

	atomic_or(&alloc_mask, BIT(chan));
}

int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);
	static const enum nrf_lfclk_start_mode mode =
		IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT) ?
			CLOCK_CONTROL_NRF_LF_START_NOWAIT :
			(IS_ENABLED(CONFIG_SYSTEM_CLOCK_WAIT_FOR_AVAILABILITY) ?
			CLOCK_CONTROL_NRF_LF_START_AVAILABLE :
			CLOCK_CONTROL_NRF_LF_START_STABLE);

	/* TODO: replace with counter driver to access RTC */
	nrf_rtc_prescaler_set(RTC, 0);
	for (uint32_t chan = 0; chan < CHAN_COUNT; chan++) {
		nrf_rtc_int_enable(RTC, RTC_CHANNEL_INT_MASK(chan));
	}

	NVIC_ClearPendingIRQ(RTC_IRQn);

	IRQ_CONNECT(RTC_IRQn, DT_IRQ(DT_NODELABEL(RTC_LABEL), priority),
		    rtc_nrf_isr, 0, 0);
	irq_enable(RTC_IRQn);

	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_CLEAR);
	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_START);

	int_mask = BIT_MASK(CHAN_COUNT);
	if (CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT) {
		alloc_mask = BIT_MASK(EXT_CHAN_COUNT) << 1;
	}

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		compare_set(0, counter() + CYC_PER_TICK,
			    sys_clock_timeout_handler, NULL);
	}

	z_nrf_clock_control_lf_on(mode);

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
	compare_set(0, cyc, sys_clock_timeout_handler, NULL);
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

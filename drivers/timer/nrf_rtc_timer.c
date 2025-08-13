/*
 * Copyright (c) 2016-2021 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/timer/nrf_rtc_timer.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/barrier.h>
#include <haly/nrfy_rtc.h>
#include <zephyr/irq.h>

#define RTC_BIT_WIDTH 24

#if (CONFIG_NRF_RTC_COUNTER_BIT_WIDTH < RTC_BIT_WIDTH)
#define CUSTOM_COUNTER_BIT_WIDTH 1
#define WRAP_CH 0
#define SYS_CLOCK_CH 1
#include "nrfx_ppi.h"
#else
#define CUSTOM_COUNTER_BIT_WIDTH 0
#define SYS_CLOCK_CH 0
#endif

#define RTC_PRETICK (IS_ENABLED(CONFIG_SOC_NRF53_RTC_PRETICK) && \
		     IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET))

#define EXT_CHAN_COUNT CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT
#define CHAN_COUNT (EXT_CHAN_COUNT + 1 + CUSTOM_COUNTER_BIT_WIDTH)

#define RTC NRF_RTC1
#define RTC_IRQn NRFX_IRQ_NUMBER_GET(RTC)
#define RTC_LABEL rtc1
#define CHAN_COUNT_MAX (RTC1_CC_NUM - (RTC_PRETICK ? 1 : 0))

BUILD_ASSERT(CHAN_COUNT <= CHAN_COUNT_MAX, "Not enough compare channels");
/* Ensure that counter driver for RTC1 is not enabled. */
BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(RTC_LABEL), disabled),
	     "Counter for RTC1 must be disabled");

#define COUNTER_BIT_WIDTH CONFIG_NRF_RTC_COUNTER_BIT_WIDTH
#define COUNTER_SPAN BIT(COUNTER_BIT_WIDTH)
#define COUNTER_MAX (COUNTER_SPAN - 1U)
#define COUNTER_HALF_SPAN (COUNTER_SPAN / 2U)
#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((COUNTER_HALF_SPAN - CYC_PER_TICK) / CYC_PER_TICK)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

#define OVERFLOW_RISK_RANGE_END (COUNTER_SPAN / 16)
#define ANCHOR_RANGE_START (COUNTER_SPAN / 8)
#define ANCHOR_RANGE_END (7 * COUNTER_SPAN / 8)
#define TARGET_TIME_INVALID (UINT64_MAX)

extern void rtc_pretick_rtc1_isr_hook(void);

static volatile uint32_t overflow_cnt;
static volatile uint64_t anchor;
static uint64_t last_count;
static bool sys_busy;

struct z_nrf_rtc_timer_chan_data {
	z_nrf_rtc_timer_compare_handler_t callback;
	void *user_context;
	volatile uint64_t target_time;
};

static struct z_nrf_rtc_timer_chan_data cc_data[CHAN_COUNT];
static atomic_t int_mask;
static atomic_t alloc_mask;
static atomic_t force_isr_mask;

static uint32_t counter_sub(uint32_t a, uint32_t b)
{
	return (a - b) & COUNTER_MAX;
}

static void set_comparator(int32_t chan, uint32_t cyc)
{
	nrfy_rtc_cc_set(RTC, chan, cyc & COUNTER_MAX);
}

static bool event_check(int32_t chan)
{
	return nrfy_rtc_event_check(RTC, NRF_RTC_CHANNEL_EVENT_ADDR(chan));
}

static void event_clear(int32_t chan)
{
	nrfy_rtc_event_clear(RTC, NRF_RTC_CHANNEL_EVENT_ADDR(chan));
}

static void event_enable(int32_t chan)
{
	nrfy_rtc_event_enable(RTC, NRF_RTC_CHANNEL_INT_MASK(chan));
}

static void event_disable(int32_t chan)
{
	nrfy_rtc_event_disable(RTC, NRF_RTC_CHANNEL_INT_MASK(chan));
}

static uint32_t counter(void)
{
	return nrfy_rtc_counter_get(RTC);
}

static uint32_t absolute_time_to_cc(uint64_t absolute_time)
{
	/* 24 least significant bits represent target CC value */
	return absolute_time & COUNTER_MAX;
}

static uint32_t full_int_lock(void)
{
	uint32_t mcu_critical_state;

	if (IS_ENABLED(CONFIG_NRF_RTC_TIMER_LOCK_ZERO_LATENCY_IRQS)) {
		mcu_critical_state = __get_PRIMASK();
		__disable_irq();
	} else {
		mcu_critical_state = irq_lock();
	}

	return mcu_critical_state;
}

static void full_int_unlock(uint32_t mcu_critical_state)
{
	if (IS_ENABLED(CONFIG_NRF_RTC_TIMER_LOCK_ZERO_LATENCY_IRQS)) {
		__set_PRIMASK(mcu_critical_state);
	} else {
		irq_unlock(mcu_critical_state);
	}
}

uint32_t z_nrf_rtc_timer_compare_evt_address_get(int32_t chan)
{
	__ASSERT_NO_MSG(chan >= 0 && chan < CHAN_COUNT);
	return nrfy_rtc_event_address_get(RTC, nrfy_rtc_compare_event_get(chan));
}

uint32_t z_nrf_rtc_timer_capture_task_address_get(int32_t chan)
{
#if defined(RTC_TASKS_CAPTURE_TASKS_CAPTURE_Msk)
	__ASSERT_NO_MSG(chan >= 0 && chan < CHAN_COUNT);
	if (chan == SYS_CLOCK_CH) {
		return 0;
	}

	return nrfy_rtc_task_address_get(RTC, nrfy_rtc_capture_task_get(chan));
#else
	ARG_UNUSED(chan);
	return 0;
#endif
}

static bool compare_int_lock(int32_t chan)
{
	atomic_val_t prev = atomic_and(&int_mask, ~BIT(chan));

	nrfy_rtc_int_disable(RTC, NRF_RTC_CHANNEL_INT_MASK(chan));

	barrier_dmem_fence_full();
	barrier_isync_fence_full();

	return prev & BIT(chan);
}


bool z_nrf_rtc_timer_compare_int_lock(int32_t chan)
{
	__ASSERT_NO_MSG(chan > 0 && chan < CHAN_COUNT);

	return compare_int_lock(chan);
}

static void compare_int_unlock(int32_t chan, bool key)
{
	if (key) {
		atomic_or(&int_mask, BIT(chan));
		nrfy_rtc_int_enable(RTC, NRF_RTC_CHANNEL_INT_MASK(chan));
		if (atomic_get(&force_isr_mask) & BIT(chan)) {
			NVIC_SetPendingIRQ(RTC_IRQn);
		}
	}
}

void z_nrf_rtc_timer_compare_int_unlock(int32_t chan, bool key)
{
	__ASSERT_NO_MSG(chan > 0 && chan < CHAN_COUNT);

	compare_int_unlock(chan, key);
}

uint32_t z_nrf_rtc_timer_compare_read(int32_t chan)
{
	__ASSERT_NO_MSG(chan >= 0 && chan < CHAN_COUNT);

	return nrfy_rtc_cc_get(RTC, chan);
}

uint64_t z_nrf_rtc_timer_get_ticks(k_timeout_t t)
{
	int64_t abs_ticks;

	abs_ticks = Z_TICK_ABS(t.ticks);
	if (Z_IS_TIMEOUT_RELATIVE(t)) {
		return (t.ticks > COUNTER_SPAN) ?
			-EINVAL : (z_nrf_rtc_timer_read() + t.ticks * CYC_PER_TICK);
	}

	/* absolute timeout */
	/* abs_ticks is int64_t so it has 63 bits. If CYC_PER_TICK is <=2 then
	 * any abs_ticks will fit in 64 bits after multiplying by CYC_PER_TICK
	 * but if CYC_PER_TICK is higher then it is possible that abs_ticks cannot
	 * be converted to RTC ticks and check for overflow is needed.
	 */
	if ((CYC_PER_TICK > 2) && (abs_ticks > (UINT64_MAX / CYC_PER_TICK))) {
		return -EINVAL;
	}

	return abs_ticks * CYC_PER_TICK;
}

/** @brief Function safely sets an alarm.
 *
 * It assumes that provided value is at most COUNTER_HALF_SPAN cycles from now
 * (other values are considered to be from the past). It detects late setting
 * and properly adjusts CC values that are too near in the future to guarantee
 * triggering a COMPARE event soon, not after 512 seconds when the RTC wraps
 * around first.
 *
 * @param[in] chan A channel for which a new CC value is to be set.
 *
 * @param[in] req_cc Requested CC register value to be set.
 *
 * @param[in] exact Use @c false to allow CC adjustment if @c req_cc value is
 *                  close to the current value of the timer.
 *                  Use @c true to disallow CC adjustment. The function can
 *                  fail with -EINVAL result if @p req_cc is too close to the
 *                  current value.
 *
 * @retval 0 The requested CC has been set successfully.
 * @retval -EINVAL The requested CC value could not be reliably set.
 */
static int set_alarm(int32_t chan, uint32_t req_cc, bool exact)
{
	int ret = 0;

	/* Ensure that the value exposed in this driver API is consistent with
	 * assumptions of this function.
	 */
	BUILD_ASSERT(NRF_RTC_TIMER_MAX_SCHEDULE_SPAN <= COUNTER_HALF_SPAN);

	/* According to product specifications, when the current counter value
	 * is N, a value of N+2 written to the CC register is guaranteed to
	 * trigger a COMPARE event at N+2, but tests show that this compare
	 * value can be missed when the previous CC value is N+1 and the write
	 * occurs in the second half of the RTC clock cycle (such situation can
	 * be provoked by test_next_cycle_timeouts in the nrf_rtc_timer suite).
	 * This never happens when the written value is N+3. Use 3 cycles as
	 * the nearest possible scheduling then.
	 */
	enum { MIN_CYCLES_FROM_NOW = 3 };
	uint32_t cc_val = req_cc;
	uint32_t cc_inc = MIN_CYCLES_FROM_NOW;

	/* Disable event routing for the channel to avoid getting a COMPARE
	 * event for the previous CC value before the new one takes effect
	 * (however, even if such spurious event was generated, it would be
	 * properly filtered out in process_channel(), where the target time
	 * is checked).
	 * Clear also the event as it may already be generated at this point.
	 */
	event_disable(chan);
	event_clear(chan);

	for (;;) {
		uint32_t now;

#if CUSTOM_COUNTER_BIT_WIDTH
		/* If a CC value is 0 when a CLEAR task is set, this will not
		 * trigger a COMAPRE event. Need to use 1 instead.
		 */
		if ((cc_val & COUNTER_MAX) == 0) {
			cc_val = 1;
		}
#endif
		set_comparator(chan, cc_val);
		/* Enable event routing after the required CC value was set.
		 * Even though the above operation may get repeated (see below),
		 * there is no need to disable event routing in every iteration
		 * of the loop, as the COMPARE event resulting from any attempt
		 * of setting the CC register is acceptable (as mentioned above,
		 * process_channel() does the proper filtering).
		 */
		event_enable(chan);

		now = counter();

		/* Check if the CC register was successfully set to a value
		 * that will for sure trigger a COMPARE event as expected.
		 * If not, try again, adjusting the CC value accordingly.
		 * Increase the CC value by a larger number of cycles in each
		 * trial to avoid spending too much time in this loop if it
		 * continuously gets interrupted and delayed by something.
		 */
		if (counter_sub(cc_val, now + MIN_CYCLES_FROM_NOW) >
		    (COUNTER_HALF_SPAN - MIN_CYCLES_FROM_NOW)) {
			/* If the COMPARE event turns out to be already
			 * generated, check if the loop can be finished.
			 */
			if (event_check(chan)) {
				/* If the current counter value has not yet
				 * reached the requested CC value, the event
				 * must come from the previously set CC value
				 * (the alarm is apparently rescheduled).
				 * The event needs to be cleared then and the
				 * loop needs to be continued.
				 */
				now = counter();
				if (counter_sub(now, req_cc) > COUNTER_HALF_SPAN) {
					event_clear(chan);
					if (exact) {
						ret = -EINVAL;
						break;
					}
				} else {
					break;
				}
			} else if (exact) {
				ret = -EINVAL;
				break;
			}

			cc_val = now + cc_inc;
			cc_inc++;
		} else {
			break;
		}
	}

	return ret;
}

static int compare_set_nolocks(int32_t chan, uint64_t target_time,
			z_nrf_rtc_timer_compare_handler_t handler,
			void *user_data, bool exact)
{
	int ret = 0;
	uint32_t cc_value = absolute_time_to_cc(target_time);
	uint64_t curr_time = z_nrf_rtc_timer_read();

	if (curr_time < target_time) {
		if (target_time - curr_time > COUNTER_HALF_SPAN) {
			/* Target time is too distant. */
			return -EINVAL;
		}

		if (target_time != cc_data[chan].target_time) {
			/* Target time is valid and is different than currently set.
			 * Set CC value.
			 */
			ret = set_alarm(chan, cc_value, exact);
		}
	} else if (!exact) {
		/* Force ISR handling when exiting from critical section. */
		atomic_or(&force_isr_mask, BIT(chan));
	} else {
		ret = -EINVAL;
	}

	if (ret == 0) {
		cc_data[chan].target_time = target_time;
		cc_data[chan].callback = handler;
		cc_data[chan].user_context = user_data;
	}

	return ret;
}

static int compare_set(int32_t chan, uint64_t target_time,
			z_nrf_rtc_timer_compare_handler_t handler,
			void *user_data, bool exact)
{
	bool key;

	key = compare_int_lock(chan);

	int ret = compare_set_nolocks(chan, target_time, handler, user_data, exact);

	compare_int_unlock(chan, key);

	return ret;
}

int z_nrf_rtc_timer_set(int32_t chan, uint64_t target_time,
			 z_nrf_rtc_timer_compare_handler_t handler,
			 void *user_data)
{
	__ASSERT_NO_MSG(chan > 0 && chan < CHAN_COUNT);

	return compare_set(chan, target_time, handler, user_data, false);
}

int z_nrf_rtc_timer_exact_set(int32_t chan, uint64_t target_time,
			      z_nrf_rtc_timer_compare_handler_t handler,
			      void *user_data)
{
	__ASSERT_NO_MSG(chan > 0 && chan < CHAN_COUNT);

	return compare_set(chan, target_time, handler, user_data, true);
}

void z_nrf_rtc_timer_abort(int32_t chan)
{
	__ASSERT_NO_MSG(chan > 0 && chan < CHAN_COUNT);

	bool key = compare_int_lock(chan);

	cc_data[chan].target_time = TARGET_TIME_INVALID;
	event_clear(chan);
	event_disable(chan);
	(void)atomic_and(&force_isr_mask, ~BIT(chan));

	compare_int_unlock(chan, key);
}

uint64_t z_nrf_rtc_timer_read(void)
{
	uint64_t val = ((uint64_t)overflow_cnt) << COUNTER_BIT_WIDTH;

	barrier_dmem_fence_full();

	uint32_t cntr = counter();

#if CUSTOM_COUNTER_BIT_WIDTH
	/* If counter is equal to it maximum value while val is greater
	 * than anchor, then we can assume that overflow has been recorded
	 * in the overflow_cnt, but clear task has not been triggered yet.
	 * Treat counter as if it has been cleared.
	 */
	if ((cntr == COUNTER_MAX) && (val > anchor)) {
		cntr = 0;
	}
#endif

	val += cntr;

	if (cntr < OVERFLOW_RISK_RANGE_END) {
		/* `overflow_cnt` can have incorrect value due to still unhandled overflow or
		 * due to possibility that this code preempted overflow interrupt before final write
		 * of `overflow_cnt`. Update of `anchor` occurs far in time from this moment, so
		 * `anchor` is considered valid and stable. Because of this timing there is no risk
		 * of incorrect `anchor` value caused by non-atomic read of 64-bit `anchor`.
		 */
		if (val < anchor) {
			/* Unhandled overflow, detected, let's add correction */
			val += COUNTER_SPAN;
		}
	} else {
		/* `overflow_cnt` is considered valid and stable in this range, no need to
		 * check validity using `anchor`
		 */
	}

	return val;
}

static inline bool in_anchor_range(uint32_t cc_value)
{
	return (cc_value >= ANCHOR_RANGE_START) && (cc_value < ANCHOR_RANGE_END);
}

static inline void anchor_update(uint32_t cc_value)
{
	/* Update anchor when far from overflow */
	if (in_anchor_range(cc_value)) {
		/* In this range `overflow_cnt` is considered valid and stable.
		 * Write of 64-bit `anchor` is non atomic. However it happens
		 * far in time from the moment the `anchor` is read in
		 * `z_nrf_rtc_timer_read`.
		 */
		anchor = (((uint64_t)overflow_cnt) << COUNTER_BIT_WIDTH) + cc_value;
	}
}

static void sys_clock_timeout_handler(int32_t chan,
				      uint64_t expire_time,
				      void *user_data)
{
	uint32_t cc_value = absolute_time_to_cc(expire_time);
	uint32_t dticks = (uint32_t)(expire_time - last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	anchor_update(cc_value);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* protection is not needed because we are in the RTC interrupt
		 * so it won't get preempted by the interrupt.
		 */
		compare_set(chan, last_count + CYC_PER_TICK,
					  sys_clock_timeout_handler, NULL, false);
	}

	sys_clock_announce(dticks);
}

static bool channel_processing_check_and_clear(int32_t chan)
{
	if (nrfy_rtc_int_enable_check(RTC, NRF_RTC_CHANNEL_INT_MASK(chan))) {
		/* The processing of channel can be caused by CC match
		 * or be forced.
		 */
		if ((atomic_and(&force_isr_mask, ~BIT(chan)) & BIT(chan)) ||
		    event_check(chan)) {
			event_clear(chan);
			return true;
		}
	}

	return false;
}

static void process_channel(int32_t chan)
{
	if (channel_processing_check_and_clear(chan)) {
		void *user_context;
		uint32_t mcu_critical_state;
		uint64_t curr_time;
		uint64_t expire_time;
		z_nrf_rtc_timer_compare_handler_t handler = NULL;

		curr_time = z_nrf_rtc_timer_read();

		/* This critical section is used to provide atomic access to
		 * cc_data structure and prevent higher priority contexts
		 * (including ZLIs) from overwriting it.
		 */
		mcu_critical_state = full_int_lock();

		/* If target_time is in the past or is equal to current time
		 * value, execute the handler.
		 */
		expire_time = cc_data[chan].target_time;
		if (curr_time >= expire_time) {
			handler = cc_data[chan].callback;
			user_context = cc_data[chan].user_context;
			cc_data[chan].callback = NULL;
			cc_data[chan].target_time = TARGET_TIME_INVALID;
			event_disable(chan);
			/* Because of the way set_alarm() sets the CC register,
			 * it may turn out that another COMPARE event has been
			 * generated for the same alarm. Make sure the event
			 * is cleared, so that the ISR is not executed again
			 * unnecessarily.
			 */
			event_clear(chan);
		}

		full_int_unlock(mcu_critical_state);

		if (handler) {
			handler(chan, expire_time, user_context);
		}
	}
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

	if (RTC_PRETICK) {
		rtc_pretick_rtc1_isr_hook();
	}

#if CUSTOM_COUNTER_BIT_WIDTH
	if (nrfy_rtc_int_enable_check(RTC, NRF_RTC_CHANNEL_INT_MASK(WRAP_CH)) &&
	    nrfy_rtc_events_process(RTC, NRF_RTC_CHANNEL_INT_MASK(WRAP_CH))) {
#else
	if (nrfy_rtc_int_enable_check(RTC, NRF_RTC_INT_OVERFLOW_MASK) &&
	    nrfy_rtc_events_process(RTC, NRF_RTC_INT_OVERFLOW_MASK)) {
#endif
		overflow_cnt++;
	}

	for (int32_t chan = SYS_CLOCK_CH; chan < CHAN_COUNT; chan++) {
		process_channel(chan);
	}
}

int32_t z_nrf_rtc_timer_chan_alloc(void)
{
	int32_t chan;
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

void z_nrf_rtc_timer_chan_free(int32_t chan)
{
	__ASSERT_NO_MSG(chan > 0 && chan < CHAN_COUNT);

	atomic_or(&alloc_mask, BIT(chan));
}


int z_nrf_rtc_timer_trigger_overflow(void)
{
	uint32_t mcu_critical_state;
	int err = 0;

	if (!IS_ENABLED(CONFIG_NRF_RTC_TIMER_TRIGGER_OVERFLOW) ||
	    (CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT > 0)) {
		return -ENOTSUP;
	}

	mcu_critical_state = full_int_lock();
	if (sys_busy) {
		err = -EBUSY;
		goto bail;
	}

	if (counter() >= (COUNTER_SPAN - 100)) {
		err = -EAGAIN;
		goto bail;
	}

	nrfy_rtc_task_trigger(RTC, NRF_RTC_TASK_TRIGGER_OVERFLOW);
	k_busy_wait(80);

	uint64_t now = z_nrf_rtc_timer_read();

	if (err == 0) {
		sys_clock_timeout_handler(SYS_CLOCK_CH, now, NULL);
	}
bail:
	full_int_unlock(mcu_critical_state);

	return err;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
	uint32_t cyc;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		cyc = MAX_TICKS * CYC_PER_TICK;
		sys_busy = false;
	} else {
		/* Value of ticks can be zero or negative, what means "announce
		 * the next tick" (the same as ticks equal to 1).
		 */
		cyc = CLAMP(ticks, 1, (int32_t)MAX_TICKS);
		cyc *= CYC_PER_TICK;
		sys_busy = true;
	}

	uint32_t unannounced = z_nrf_rtc_timer_read() - last_count;

	/* If we haven't announced for more than half the 24-bit wrap
	 * duration, then force an announce to avoid loss of a wrap
	 * event.  This can happen if new timeouts keep being set
	 * before the existing one triggers the interrupt.
	 */
	if (unannounced >= COUNTER_HALF_SPAN) {
		cyc = 0;
	}

	/* Get the cycles from last_count to the tick boundary after
	 * the requested ticks have passed starting now.
	 */
	cyc += unannounced;
	cyc = DIV_ROUND_UP(cyc, CYC_PER_TICK) * CYC_PER_TICK;

	/* Due to elapsed time the calculation above might produce a
	 * duration that laps the counter.  Don't let it.
	 * This limitation also guarantees that the anchor will be properly
	 * updated before every overflow (see anchor_update()).
	 */
	if (cyc > MAX_CYCLES) {
		cyc = MAX_CYCLES;
	}

	uint64_t target_time = cyc + last_count;

	compare_set(SYS_CLOCK_CH, target_time, sys_clock_timeout_handler, NULL, false);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	return (z_nrf_rtc_timer_read() - last_count) / CYC_PER_TICK;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)z_nrf_rtc_timer_read();
}

static void int_event_disable_rtc(void)
{
	uint32_t mask = NRF_RTC_INT_TICK_MASK     |
#if !CUSTOM_COUNTER_BIT_WIDTH
			NRF_RTC_INT_OVERFLOW_MASK |
#endif
			NRF_RTC_INT_COMPARE0_MASK |
			NRF_RTC_INT_COMPARE1_MASK |
			NRF_RTC_INT_COMPARE2_MASK |
			NRF_RTC_INT_COMPARE3_MASK;

	/* Reset interrupt enabling to expected reset values */
	nrfy_rtc_int_disable(RTC, mask);

	/* Reset event routing enabling to expected reset values */
	nrfy_rtc_event_disable(RTC, mask);
}

void sys_clock_disable(void)
{
	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_STOP);
	irq_disable(RTC_IRQn);
	int_event_disable_rtc();
	NVIC_ClearPendingIRQ(RTC_IRQn);
}

static int sys_clock_driver_init(void)
{
	int_event_disable_rtc();

	/* TODO: replace with counter driver to access RTC */
	nrfy_rtc_prescaler_set(RTC, 0);
	for (int32_t chan = 0; chan < CHAN_COUNT; chan++) {
		cc_data[chan].target_time = TARGET_TIME_INVALID;
		nrfy_rtc_int_enable(RTC, NRF_RTC_CHANNEL_INT_MASK(chan));
	}

#if !CUSTOM_COUNTER_BIT_WIDTH
	nrfy_rtc_int_enable(RTC, NRF_RTC_INT_OVERFLOW_MASK);
#endif

	NVIC_ClearPendingIRQ(RTC_IRQn);

	IRQ_CONNECT(RTC_IRQn, DT_IRQ(DT_NODELABEL(RTC_LABEL), priority),
		    rtc_nrf_isr, 0, 0);
	irq_enable(RTC_IRQn);

	nrfy_rtc_task_trigger(RTC, NRF_RTC_TASK_CLEAR);
	nrfy_rtc_task_trigger(RTC, NRF_RTC_TASK_START);

	int_mask = BIT_MASK(CHAN_COUNT);
	if (CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT) {
		alloc_mask = BIT_MASK(CHAN_COUNT) & ~BIT(SYS_CLOCK_CH);
	}

	uint32_t initial_timeout = IS_ENABLED(CONFIG_TICKLESS_KERNEL) ?
		MAX_CYCLES : CYC_PER_TICK;

	compare_set(SYS_CLOCK_CH, initial_timeout, sys_clock_timeout_handler, NULL, false);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	static const enum nrf_lfclk_start_mode mode =
		IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT) ?
			CLOCK_CONTROL_NRF_LF_START_NOWAIT :
			(IS_ENABLED(CONFIG_SYSTEM_CLOCK_WAIT_FOR_AVAILABILITY) ?
			CLOCK_CONTROL_NRF_LF_START_AVAILABLE :
			CLOCK_CONTROL_NRF_LF_START_STABLE);

	z_nrf_clock_control_lf_on(mode);
#endif

#if CUSTOM_COUNTER_BIT_WIDTH
	/* WRAP_CH reserved for wrapping. */
	alloc_mask &= ~BIT(WRAP_CH);

	nrf_rtc_event_t evt = NRF_RTC_CHANNEL_EVENT_ADDR(WRAP_CH);
	nrfx_err_t result;
	nrf_ppi_channel_t ch;

	nrfy_rtc_event_enable(RTC, NRF_RTC_CHANNEL_INT_MASK(WRAP_CH));
	nrfy_rtc_cc_set(RTC, WRAP_CH, COUNTER_MAX);
	uint32_t evt_addr;
	uint32_t task_addr;

	evt_addr = nrfy_rtc_event_address_get(RTC, evt);
	task_addr = nrfy_rtc_task_address_get(RTC, NRF_RTC_TASK_CLEAR);

	result = nrfx_ppi_channel_alloc(&ch);
	if (result != NRFX_SUCCESS) {
		return -ENODEV;
	}
	(void)nrfx_ppi_channel_assign(ch, evt_addr, task_addr);
	(void)nrfx_ppi_channel_enable(ch);
#endif
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

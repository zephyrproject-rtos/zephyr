/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform/nrf_802154_lp_timer.h"

#include <assert.h>
#include <kernel.h>
#include <stdbool.h>

#include "drivers/timer/nrf_rtc_timer.h"

#include "platform/nrf_802154_clock.h"
#include "nrf_802154_sl_utils.h"

struct timer_desc {
	z_nrf_rtc_timer_compare_handler_t handler;
	uint64_t target_time;
	int32_t chan;
	int32_t int_lock_key;
	bool is_running;
};

static struct timer_desc m_timer;
static struct timer_desc m_sync_timer;
static volatile bool m_clock_ready;
static bool m_in_critical_section;

/**
 * @brief RTC IRQ handler for LP timer channel.
 */
void timer_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	(void)expire_time;
	(void)user_data;

	assert(id == m_timer.chan);

	m_timer.is_running = false;

	nrf_802154_lp_timer_fired();
}

/**
 * @brief RTC IRQ handler for synchronization timer channel.
 */
void sync_timer_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	(void)user_data;

	assert(id == m_sync_timer.chan);

	m_sync_timer.is_running = false;
	/**
	 * Expire time might have been different than the desired target
	 * time. Update it so that a more accurate value can be returned
	 * by nrf_802154_lp_timer_sync_time_get.
	 */
	m_sync_timer.target_time = NRF_802154_SL_RTC_TICKS_TO_US(expire_time);

	nrf_802154_lp_timer_synchronized();
}

/**
 * @brief Convert 32-bit target time to absolute 64-bit target time.
 */
uint64_t target_time_convert_to_64_bits(uint32_t t0, uint32_t dt)
{
	/**
	 * Target time is provided as two 32-bit integers defining a moment in time
	 * in microsecond domain. In order to use bit-shifting instead of modulo
	 * division, calculations are performed in microsecond domain, not in RTC ticks.
	 *
	 * Maximum possible value of target time specified with two 32-bit unsigned
	 * integers is equal 2 * (2^32 - 1), or 2^33 - 2. However, instead of extending
	 * the bit-width to 64 to fit that number, let the addition of t0 and dt
	 * overflow at 32 bits.
	 *
	 * The target time can point to a moment in the future, but can be overdue
	 * as well. In order to determine what's the case and correctly set the
	 * absolute target time, it's necessary to compare the least significant
	 * 32 bits of the current time, 64-bit time with the provided 32-bit target
	 * time. Let's assume that half of the 32-bit range can be used for specifying
	 * target times in the future, and the other half - in the past.
	 */
	uint64_t now_us = NRF_802154_SL_RTC_TICKS_TO_US(z_nrf_rtc_timer_read());
	uint32_t now_us_wrapped = (uint32_t)now_us;
	uint32_t target_time_us_wrapped = (uint32_t)(t0 + dt);
	uint32_t time_diff = target_time_us_wrapped - now_us_wrapped;
	uint64_t result = UINT64_C(0);

	if (time_diff < 0x80000000) {
		/**
		 * Target time is assumed to be in the future. Check if a 32-bit overflow
		 * occurs between the current time and the target time.
		 */
		if (now_us_wrapped > target_time_us_wrapped) {
			/**
			 * Add a 32-bit overflow and replace the least significant 32 bits
			 * with the provided target time.
			 */
			result = now_us + UINT32_MAX + 1;
			result &= ~(uint64_t)UINT32_MAX;
			result |= target_time_us_wrapped;
		} else {
			/**
			 * Leave the most significant 32 bits and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = (now_us & (~(uint64_t)UINT32_MAX)) | target_time_us_wrapped;
		}
	} else {
		/**
		 * Target time is assumed to be in the past. Check if a 32-bit overflow
		 * occurs between the target time and the current time.
		 */
		if (now_us_wrapped > target_time_us_wrapped) {
			/**
			 * Leave the most significant 32 bits and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = (now_us & (~(uint64_t)UINT32_MAX)) | target_time_us_wrapped;
		} else {
			/**
			 * Subtract a 32-bit overflow and replace the least significant
			 * 32 bits with the provided target time.
			 */
			result = now_us - (UINT32_MAX + 1);
			result &= ~(uint64_t)UINT32_MAX;
			result |= target_time_us_wrapped;
		}
	}

	/* Convert the result from microseconds domain to RTC ticks domain. */
	return NRF_802154_SL_US_TO_RTC_TICKS(result);
}

/**
 * @brief Start one-shot timer that expires at specified time on desired channel.
 *
 * Start one-shot timer that will expire when @p target_time is reached on channel @p channel.
 *
 * @param[inout] timer        Pointer to descriptor of timer to set.
 * @param[in]    target_time  Absolute target time in microseconds.
 */
static void timer_start_at(struct timer_desc *timer, uint64_t target_time)
{
	nrf_802154_sl_mcu_critical_state_t state;

	z_nrf_rtc_timer_set(timer->chan, target_time, timer->handler, NULL);

	nrf_802154_sl_mcu_critical_enter(state);

	timer->is_running = true;
	timer->target_time = NRF_802154_SL_RTC_TICKS_TO_US(target_time);

	nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_clock_lfclk_ready(void)
{
	m_clock_ready = true;
}

void nrf_802154_lp_timer_init(void)
{
	m_in_critical_section = false;
	m_timer.is_running = false;
	m_timer.handler = timer_handler;
	m_sync_timer.is_running = false;
	m_sync_timer.handler = sync_timer_handler;

	/* Setup low frequency clock. */
	nrf_802154_clock_lfclk_start();

	while (!m_clock_ready) {
		/* Intentionally empty */
	}

	m_timer.chan = z_nrf_rtc_timer_chan_alloc();
	if (m_timer.chan < 0) {
		assert(false);
		return;
	}

	m_sync_timer.chan = z_nrf_rtc_timer_chan_alloc();
	if (m_sync_timer.chan < 0) {
		assert(false);
		return;
	}
}

void nrf_802154_lp_timer_deinit(void)
{
	(void)z_nrf_rtc_timer_compare_int_lock(m_timer.chan);

	nrf_802154_lp_timer_sync_stop();

	z_nrf_rtc_timer_chan_free(m_timer.chan);
	z_nrf_rtc_timer_chan_free(m_sync_timer.chan);

	nrf_802154_clock_lfclk_stop();
}

void nrf_802154_lp_timer_critical_section_enter(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	if (!m_in_critical_section) {
		m_timer.int_lock_key = z_nrf_rtc_timer_compare_int_lock(m_timer.chan);
		m_sync_timer.int_lock_key = z_nrf_rtc_timer_compare_int_lock(m_sync_timer.chan);
		m_in_critical_section = true;
	}

	nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_lp_timer_critical_section_exit(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	m_in_critical_section = false;

	z_nrf_rtc_timer_compare_int_unlock(m_timer.chan, m_timer.int_lock_key);
	z_nrf_rtc_timer_compare_int_unlock(m_sync_timer.chan, m_sync_timer.int_lock_key);

	nrf_802154_sl_mcu_critical_exit(state);
}

uint32_t nrf_802154_lp_timer_time_get(void)
{
	return (uint32_t)NRF_802154_SL_RTC_TICKS_TO_US(z_nrf_rtc_timer_read());
}

uint32_t nrf_802154_lp_timer_granularity_get(void)
{
	return NRF_802154_SL_US_PER_TICK;
}

void nrf_802154_lp_timer_start(uint32_t t0, uint32_t dt)
{
	uint64_t target_time = target_time_convert_to_64_bits(t0, dt);

	timer_start_at(&m_timer, target_time);
}

bool nrf_802154_lp_timer_is_running(void)
{
	/* We're not interested here if synchronization timer is running. */
	return m_timer.is_running;
}

void nrf_802154_lp_timer_stop(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	m_timer.is_running = false;

	nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_lp_timer_sync_start_now(void)
{
	uint64_t now = z_nrf_rtc_timer_read();

	/**
	 * Despite this function's name, synchronization is not expected to be
	 * scheduled for the current tick. Add a safe 3-tick margin
	 */
	timer_start_at(&m_sync_timer, now + 3);
}

void nrf_802154_lp_timer_sync_start_at(uint32_t t0, uint32_t dt)
{
	uint64_t target_time = target_time_convert_to_64_bits(t0, dt);

	timer_start_at(&m_sync_timer, target_time);
}

void nrf_802154_lp_timer_sync_stop(void)
{
	z_nrf_rtc_timer_abort(m_sync_timer.chan);
}

uint32_t nrf_802154_lp_timer_sync_event_get(void)
{
	return z_nrf_rtc_timer_compare_evt_address_get(m_sync_timer.chan);
}

uint32_t nrf_802154_lp_timer_sync_time_get(void)
{
	return m_sync_timer.target_time;
}

#ifndef UNITY_ON_TARGET
__WEAK void nrf_802154_lp_timer_synchronized(void)
{
	/* Intentionally empty */
}

#endif

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform/lp_timer/nrf_802154_lp_timer.h"

#include <assert.h>
#include <kernel.h>
#include <stdbool.h>

#include "drivers/timer/nrf_rtc_timer.h"

#include "platform/clock/nrf_802154_clock.h"
#include "nrf_802154_sl_utils.h"

static volatile bool m_clock_ready;
static bool m_is_running;
static uint32_t m_rtc_channel;
static bool m_in_critical_section;

void rtc_irq_handler(uint32_t id, uint32_t cc_value, void *user_data)
{
	(void)cc_value;
	(void)user_data;
	nrf_802154_sl_mcu_critical_state_t state;

	assert(id == m_rtc_channel);

	nrf_802154_sl_mcu_critical_enter(state);

	m_is_running = false;
	(void)z_nrf_rtc_timer_compare_int_lock(m_rtc_channel);

	nrf_802154_sl_mcu_critical_exit(state);

	nrf_802154_lp_timer_fired();
}

/**
 * @brief Start one-shot timer that expires at specified time on desired channel.
 *
 * Start one-shot timer that will expire @p dt microseconds after @p t0 time on channel @p channel.
 *
 * @param[in]  channel  Compare channel on which timer will be started.
 * @param[in]  t0       Number of microseconds representing timer start time.
 * @param[in]  dt       Time of timer expiration as time elapsed from @p t0 [us].
 */
static void timer_start_at(uint32_t channel,
			   uint32_t t0,
			   uint32_t dt)
{
	uint32_t cc_value = NRF_802154_SL_US_TO_RTC_TICKS(t0 + dt);
	nrf_802154_sl_mcu_critical_state_t state;

	z_nrf_rtc_timer_compare_set(m_rtc_channel, cc_value, rtc_irq_handler, NULL);

	nrf_802154_sl_mcu_critical_enter(state);

	m_is_running = true;
	z_nrf_rtc_timer_compare_int_unlock(m_rtc_channel, (m_in_critical_section == false));

	nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_clock_lfclk_ready(void)
{
	m_clock_ready = true;
}

void nrf_802154_lp_timer_init(void)
{
	m_in_critical_section = false;
	m_is_running = false;

	/* Setup low frequency clock. */
	nrf_802154_clock_lfclk_start();

	while (!m_clock_ready) {
		/* Intentionally empty */
	}

	int32_t chan = z_nrf_rtc_timer_chan_alloc();

	if (chan >= 0) {
		m_rtc_channel = (uint32_t)chan;
	} else   {
		assert(false);
		return;
	}

	(void)z_nrf_rtc_timer_compare_int_lock(m_rtc_channel);
}

void nrf_802154_lp_timer_deinit(void)
{
	(void)z_nrf_rtc_timer_compare_int_lock(m_rtc_channel);

	z_nrf_rtc_timer_chan_free(m_rtc_channel);

	nrf_802154_clock_lfclk_stop();
}

void nrf_802154_lp_timer_critical_section_enter(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	if (!m_in_critical_section) {
		(void)z_nrf_rtc_timer_compare_int_lock(m_rtc_channel);
		m_in_critical_section = true;
	}

	nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_lp_timer_critical_section_exit(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	m_in_critical_section = false;

	z_nrf_rtc_timer_compare_int_unlock(m_rtc_channel, m_is_running);

	nrf_802154_sl_mcu_critical_exit(state);
}

uint32_t nrf_802154_lp_timer_time_get(void)
{
	/* Please note that this function does not handle RTC overflow */
	return NRF_802154_SL_RTC_TICKS_TO_US(z_nrf_rtc_timer_read());
}

uint32_t nrf_802154_lp_timer_granularity_get(void)
{
	return NRF_802154_SL_US_PER_TICK;
}

void nrf_802154_lp_timer_start(uint32_t t0, uint32_t dt)
{
	timer_start_at(m_rtc_channel, t0, dt);
}

bool nrf_802154_lp_timer_is_running(void)
{
	return m_is_running;
}

void nrf_802154_lp_timer_stop(void)
{
	nrf_802154_sl_mcu_critical_state_t state;

	nrf_802154_sl_mcu_critical_enter(state);

	m_is_running = false;
	(void)z_nrf_rtc_timer_compare_int_lock(m_rtc_channel);

	nrf_802154_sl_mcu_critical_exit(state);
}

void nrf_802154_lp_timer_sync_start_now(void)
{
	/* Currently not supported */
}

void nrf_802154_lp_timer_sync_start_at(uint32_t t0, uint32_t dt)
{
	(void)t0;
	(void)dt;
	/* Currently not supported */
}

void nrf_802154_lp_timer_sync_stop(void)
{
	/* Currently not supported */
}

uint32_t nrf_802154_lp_timer_sync_event_get(void)
{
	/* Currently not supported */
	return 0;
}

uint32_t nrf_802154_lp_timer_sync_time_get(void)
{
	/* Currently not supported */
	return 0;
}

#ifndef UNITY_ON_TARGET
__WEAK void nrf_802154_lp_timer_synchronized(void)
{
	/* Intentionally empty */
}

#endif

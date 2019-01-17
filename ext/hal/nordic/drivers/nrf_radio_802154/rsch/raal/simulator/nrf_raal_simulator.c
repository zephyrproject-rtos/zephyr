/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements the nrf 802.15.4 simulated radio arbiter.
 *
 * This arbiter should be used for testing driver and tweaking other arbiters.
 *
 */

#include "nrf_raal_api.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_802154_debug.h"

static bool m_continuous_requested;
static bool m_continuous_granted;

static uint16_t m_time_interval               = 250; // ms
static uint16_t m_ble_duty                    = 10;  // ms
static uint16_t m_pre_preemption_notification = 150; // us

static uint32_t m_ended_timestamp;
static uint32_t m_started_timestamp;
static uint32_t m_margin_timestamp;

static void continuous_grant(void)
{
    if (m_continuous_requested && !m_continuous_granted)
    {
        nrf_802154_pin_set(PIN_DBG_TIMESLOT_ACTIVE);
        m_continuous_granted = true;
        nrf_raal_timeslot_started();
    }
}

static void continuous_revoke(void)
{
    if (m_continuous_requested && m_continuous_granted)
    {
        nrf_802154_pin_clr(PIN_DBG_TIMESLOT_ACTIVE);
        m_continuous_granted = false;
        nrf_raal_timeslot_ended();
    }
}

static uint32_t time_get(void)
{
    NRF_TIMER0->TASKS_CAPTURE[1] = 1;
    return NRF_TIMER0->CC[1];
}

void nrf_raal_init(void)
{
    m_ended_timestamp   = m_time_interval * 1000UL;
    m_started_timestamp = m_ble_duty * 1000UL;
    m_margin_timestamp  = (m_time_interval * 1000UL) - m_pre_preemption_notification;

    NRF_MWU->PREGION[0].SUBS = 0x00000002;
    NRF_MWU->INTENSET        = MWU_INTENSET_PREGION0WA_Msk | MWU_INTENSET_PREGION0RA_Msk;

    NVIC_SetPriority(MWU_IRQn, 0);
    NVIC_ClearPendingIRQ(MWU_IRQn);
    NVIC_EnableIRQ(MWU_IRQn);

    NRF_TIMER0->MODE      = TIMER_MODE_MODE_Timer;
    NRF_TIMER0->BITMODE   = TIMER_BITMODE_BITMODE_24Bit;
    NRF_TIMER0->PRESCALER = 4;
    NRF_TIMER0->INTENSET  = TIMER_INTENSET_COMPARE0_Msk;
    NRF_TIMER0->CC[0]     = m_started_timestamp;

    NVIC_SetPriority(TIMER0_IRQn, 1);
    NVIC_ClearPendingIRQ(TIMER0_IRQn);
    NVIC_EnableIRQ(TIMER0_IRQn);

    m_continuous_requested = false;

    NRF_TIMER0->TASKS_START = 1;
}

void nrf_raal_uninit(void)
{
    // Intentionally empty.
}

void nrf_raal_continuous_mode_enter(void)
{
    uint32_t time;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_CONTINUOUS_ENTER);

    assert(!m_continuous_requested);

    m_continuous_requested = true;

    NVIC_DisableIRQ(TIMER0_IRQn);
    __DSB();
    __ISB();

    time = time_get();

    if ((time >= m_started_timestamp) && (time < m_margin_timestamp))
    {
        continuous_grant();
    }

    NVIC_EnableIRQ(TIMER0_IRQn);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_CONTINUOUS_ENTER);
}

void nrf_raal_continuous_mode_exit(void)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_CONTINUOUS_EXIT);

    assert(m_continuous_requested);

    m_continuous_requested = false;
    m_continuous_granted   = false;

    nrf_802154_pin_clr(PIN_DBG_TIMESLOT_ACTIVE);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_CONTINUOUS_EXIT);
}

void nrf_raal_continuous_ended(void)
{
    // Intentionally empty.
}

bool nrf_raal_timeslot_request(uint32_t length_us)
{
    uint32_t timer;

    assert(m_continuous_requested);

    if (!m_continuous_granted)
    {
        return false;
    }

    timer = time_get();

    return (timer >= m_started_timestamp) && ((timer + length_us) < m_margin_timestamp);
}

uint32_t nrf_raal_timeslot_us_left_get(void)
{
    uint32_t timer = time_get();

    return ((timer >= m_started_timestamp) && (timer < m_margin_timestamp)) ?
           (m_margin_timestamp - timer) : 0;
}

void TIMER0_IRQHandler(void)
{
    uint32_t ev_timestamp;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_HANDLER);

    if (NRF_TIMER0->EVENTS_COMPARE[0])
    {
        while (time_get() >= NRF_TIMER0->CC[0])
        {
            NRF_TIMER0->EVENTS_COMPARE[0] = 0;

            ev_timestamp = NRF_TIMER0->CC[0];

            if (ev_timestamp == m_ended_timestamp)
            {
                nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_ENDED);

                NRF_MWU->REGIONENSET = MWU_REGIONENSET_PRGN0WA_Msk | MWU_REGIONENSET_PRGN0RA_Msk;

                NRF_TIMER0->TASKS_STOP  = 1;
                NRF_TIMER0->TASKS_CLEAR = 1;

                NRF_TIMER0->CC[0] = m_started_timestamp;

                NRF_TIMER0->TASKS_START = 1;

                nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_ENDED);
            }
            else if (ev_timestamp == m_started_timestamp)
            {
                nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_START);

                NRF_MWU->REGIONENCLR = MWU_REGIONENCLR_PRGN0WA_Msk | MWU_REGIONENCLR_PRGN0RA_Msk;

                NRF_TIMER0->CC[0] = m_margin_timestamp;

                continuous_grant();

                nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_START);
            }
            else if (ev_timestamp == m_margin_timestamp)
            {
                nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RAAL_SIG_EVENT_MARGIN);

                NRF_TIMER0->CC[0] = m_ended_timestamp;

                continuous_revoke();

                nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_EVENT_MARGIN);
            }
            else
            {
                assert(false);
            }
        }
    }

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RAAL_SIG_HANDLER);
}

void MWU_IRQHandler(void)
{
    assert(false);
}

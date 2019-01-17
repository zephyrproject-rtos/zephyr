/* Copyright (c) 2018, Nordic Semiconductor ASA
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
 *   This file implements ACK timeout procedure for the 802.15.4 driver.
 *
 */

#include "nrf_802154_ack_timeout.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_notification.h"
#include "nrf_802154_procedures_duration.h"
#include "nrf_802154_request.h"
#include "timer_scheduler/nrf_802154_timer_sched.h"

#define RETRY_DELAY     500     ///< Procedure is delayed by this time if it cannot be performed at the moment [us].
#define MAX_RETRY_DELAY 1000000 ///< Maximum allowed delay of procedure retry [us].

static void timeout_timer_retry(void);

static uint32_t           m_timeout = NRF_802154_PRECISE_ACK_TIMEOUT_DEFAULT_TIMEOUT; ///< ACK timeout in us.
static nrf_802154_timer_t m_timer;                                                    ///< Timer used to notify when the ACK frama is not received for too long.
static volatile bool      m_procedure_is_active;
static const uint8_t    * mp_frame;

static void notify_tx_error(bool result)
{
    if (result)
    {
        nrf_802154_notify_transmit_failed(mp_frame, NRF_802154_TX_ERROR_NO_ACK);
    }
}

static void timeout_timer_fired(void * p_context)
{
    (void)p_context;

    if (m_procedure_is_active)
    {
        if (nrf_802154_request_receive(NRF_802154_TERM_802154,
                                       REQ_ORIG_ACK_TIMEOUT,
                                       notify_tx_error,
                                       false))
        {
            m_procedure_is_active = false;
        }
        else
        {
            timeout_timer_retry();
        }
    }
}

static void timeout_timer_retry(void)
{
    m_timer.dt += RETRY_DELAY;
    assert(m_timer.dt <= MAX_RETRY_DELAY);

    nrf_802154_timer_sched_add(&m_timer, true);
}

static void timeout_timer_start(void)
{
    m_timer.callback  = timeout_timer_fired;
    m_timer.p_context = NULL;
    m_timer.t0        = nrf_802154_timer_sched_time_get();
    m_timer.dt        = m_timeout +
                        IMM_ACK_DURATION +
                        nrf_802154_frame_duration_get(mp_frame[0], false, true);

    m_procedure_is_active = true;

    nrf_802154_timer_sched_add(&m_timer, true);
}

static void timeout_timer_stop(void)
{
    m_procedure_is_active = false;

    // To make sure `timeout_timer_fired()` detects that procedure is being stopped if it preempts
    // this function.
    __DMB();

    nrf_802154_timer_sched_remove(&m_timer);
}

void nrf_802154_ack_timeout_time_set(uint32_t time)
{
    m_timeout = time;
}

bool nrf_802154_ack_timeout_tx_started_hook(const uint8_t * p_frame)
{
    mp_frame = p_frame;
    timeout_timer_start();

    return true;
}

bool nrf_802154_ack_timeout_abort(nrf_802154_term_t term_lvl, req_originator_t req_orig)
{
    bool result;

    if (!m_procedure_is_active || req_orig == REQ_ORIG_ACK_TIMEOUT)
    {
        // Ignore if procedure is not running or self-request.
        result = true;
    }
    else if (term_lvl >= NRF_802154_TERM_802154)
    {
        // Stop procedure only if termination level is high enough.
        timeout_timer_stop();

        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

void nrf_802154_ack_timeout_transmitted_hook(const uint8_t * p_frame)
{
    assert((p_frame == mp_frame) || (!m_procedure_is_active));

    timeout_timer_stop();
}

bool nrf_802154_ack_timeout_tx_failed_hook(const uint8_t * p_frame, nrf_802154_tx_error_t error)
{
    (void)error;
    assert((p_frame == mp_frame) || (!m_procedure_is_active));

    timeout_timer_stop();

    return true;
}

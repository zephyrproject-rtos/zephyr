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
 *   This file implements delayed transmission and reception features.
 *
 */

#include "nrf_802154_delayed_trx.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_config.h"
#include "nrf_802154_const.h"
#include "nrf_802154_notification.h"
#include "nrf_802154_pib.h"
#include "nrf_802154_procedures_duration.h"
#include "nrf_802154_request.h"
#include "nrf_802154_rsch.h"
#include "timer_scheduler/nrf_802154_timer_sched.h"

#define TX_SETUP_TIME 190u                         ///< Time [us] needed to change channel, stop rx and setup tx procedure.
#define RX_SETUP_TIME 190u                         ///< Time [us] needed to change channel, stop tx and setup rx procedure.

static const uint8_t * mp_tx_psdu;                 ///< Pointer to PHR + PSDU of the frame requested to transmit.
static bool            m_tx_cca;                   ///< If CCA should be performed prior to transmission.
static uint8_t         m_tx_channel;               ///< Channel number on which transmission should be performed.

static nrf_802154_timer_t m_timeout_timer;         ///< Timer for delayed RX timeout handling.
static uint8_t            m_rx_channel;            ///< Channel number on which reception should be performed.

static bool m_dly_op_in_progress[RSCH_DLY_TS_NUM]; ///< Status of delayed operation.

/**
 * Start delayed timeslot operation.
 *
 * @param[in]  dly_ts_id  Delayed timeslot ID.
 */
static void dly_op_start(rsch_dly_ts_id_t dly_ts_id)
{
    assert(dly_ts_id < RSCH_DLY_TS_NUM);

    m_dly_op_in_progress[dly_ts_id] = true;
}

/**
 * Stop delayed timeslot operation.
 *
 * @param[in]  dly_ts_id  Delayed timeslot ID.
 */
static void dly_op_stop(rsch_dly_ts_id_t dly_ts_id)
{
    assert(dly_ts_id < RSCH_DLY_TS_NUM);

    m_dly_op_in_progress[dly_ts_id] = false;
}

/**
 * Check if delayed operation is in progress.
 *
 * @retval true   Delayed operation is in progress (waiting or ongoing).
 * @retval false  Delayed operation is not in progress.
 */
bool dly_op_is_in_progress(rsch_dly_ts_id_t dly_ts_id)
{
    assert(dly_ts_id < RSCH_DLY_TS_NUM);

    return m_dly_op_in_progress[dly_ts_id];
}

/**
 * Notify MAC layer that requested timeslot is not granted if tx request failed.
 *
 * @param[in]  result  Result of TX request.
 */
static void notify_tx_timeslot_denied(bool result)
{
    if (!result)
    {
        nrf_802154_notify_transmit_failed(mp_tx_psdu, NRF_802154_TX_ERROR_TIMESLOT_DENIED);
    }
}

/**
 * Notify MAC layer that requested timeslot is not granted if rx request failed.
 *
 * @param[in]  result  Result of RX request.
 */
static void notify_rx_timeslot_denied(bool result)
{
    if (!result)
    {
        nrf_802154_notify_receive_failed(NRF_802154_RX_ERROR_DELAYED_TIMESLOT_DENIED);
    }
}

/**
 * Notify MAC layer that no frame was received before timeout.
 *
 * @param[in]  p_context  Not used.
 */
static void notify_rx_timeout(void * p_context)
{
    (void)p_context;

    dly_op_stop(RSCH_DLY_RX);
    nrf_802154_notify_receive_failed(NRF_802154_RX_ERROR_DELAYED_TIMEOUT);
}

/**
 * Handle TX timeslot start.
 */
static void nrf_802154_rsch_delayed_tx_timeslot_started(void)
{
    bool result;

    assert(dly_op_is_in_progress(RSCH_DLY_TX));

    nrf_802154_pib_channel_set(m_tx_channel);
    result = nrf_802154_request_channel_update();

    if (result)
    {
        result = nrf_802154_request_transmit(NRF_802154_TERM_802154,
                                             REQ_ORIG_DELAYED_TRX,
                                             mp_tx_psdu,
                                             m_tx_cca,
                                             true,
                                             notify_tx_timeslot_denied);
        (void)result;
    }
    else
    {
        notify_tx_timeslot_denied(result);
    }

    dly_op_stop(RSCH_DLY_TX);
}

/**
 * Handle RX timeslot start.
 */
static void nrf_802154_rsch_delayed_rx_timeslot_started(void)
{
    bool result;

    assert(dly_op_is_in_progress(RSCH_DLY_RX));

    nrf_802154_pib_channel_set(m_rx_channel);
    result = nrf_802154_request_channel_update();

    if (result)
    {
        result = nrf_802154_request_receive(NRF_802154_TERM_802154,
                                            REQ_ORIG_DELAYED_TRX,
                                            notify_rx_timeslot_denied,
                                            true);
        if (result)
        {
            m_timeout_timer.t0 = nrf_802154_timer_sched_time_get();

            nrf_802154_timer_sched_add(&m_timeout_timer, true);
        }
        else
        {
            dly_op_stop(RSCH_DLY_RX);
        }
    }
    else
    {
        notify_rx_timeslot_denied(result);
        dly_op_stop(RSCH_DLY_RX);
    }
}

bool nrf_802154_delayed_trx_transmit(const uint8_t * p_data,
                                     bool            cca,
                                     uint32_t        t0,
                                     uint32_t        dt,
                                     uint8_t         channel)
{
    bool     result = true;
    uint16_t timeslot_length;

    if (dly_op_is_in_progress(RSCH_DLY_TX))
    {
        result = false;
    }

    if (result)
    {
        dt -= TX_SETUP_TIME;
        dt -= TX_RAMP_UP_TIME;

        if (cca)
        {
            dt -= nrf_802154_cca_before_tx_duration_get();
        }

        mp_tx_psdu   = p_data;
        m_tx_cca     = cca;
        m_tx_channel = channel;

        timeslot_length = nrf_802154_tx_duration_get(p_data[0],
                                                     cca,
                                                     p_data[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT);

        dly_op_start(RSCH_DLY_TX);
        result = nrf_802154_rsch_delayed_timeslot_request(t0,
                                                          dt,
                                                          timeslot_length,
                                                          RSCH_PRIO_MAX,
                                                          RSCH_DLY_TX);

        if (!result)
        {
            notify_tx_timeslot_denied(result);
            dly_op_stop(RSCH_DLY_TX);
        }
    }

    return result;
}

bool nrf_802154_delayed_trx_receive(uint32_t t0,
                                    uint32_t dt,
                                    uint32_t timeout,
                                    uint8_t  channel)
{
    bool result;

    result = !dly_op_is_in_progress(RSCH_DLY_RX);

    if (result)
    {
        dt -= RX_SETUP_TIME;
        dt -= RX_RAMP_UP_TIME;

        dly_op_start(RSCH_DLY_RX);
        result = nrf_802154_rsch_delayed_timeslot_request(t0,
                                                          dt,
                                                          timeout +
                                                          nrf_802154_rx_duration_get(MAX_PACKET_SIZE,
                                                                                     true),
                                                          RSCH_PRIO_MAX,
                                                          RSCH_DLY_RX);

        if (result)
        {
            m_timeout_timer.dt        = timeout;
            m_timeout_timer.callback  = notify_rx_timeout;
            m_timeout_timer.p_context = NULL;

            m_rx_channel = channel;

        }
        else
        {
            notify_rx_timeslot_denied(result);
            dly_op_stop(RSCH_DLY_RX);
        }
    }

    return result;
}

void nrf_802154_rsch_delayed_timeslot_started(rsch_dly_ts_id_t dly_ts_id)
{
    switch (dly_ts_id)
    {
        case RSCH_DLY_TX:
            nrf_802154_rsch_delayed_tx_timeslot_started();
            break;

        case RSCH_DLY_RX:
            nrf_802154_rsch_delayed_rx_timeslot_started();
            break;

        default:
            assert(false);
    }
}

void nrf_802154_rsch_delayed_timeslot_failed(rsch_dly_ts_id_t dly_ts_id)
{
    assert(dly_ts_id < RSCH_DLY_TS_NUM);
    assert(dly_op_is_in_progress(dly_ts_id));

    if (RSCH_DLY_TX == dly_ts_id)
    {
        notify_tx_timeslot_denied(false);
    }
    else
    {
        notify_rx_timeslot_denied(false);
    }

    dly_op_stop(dly_ts_id);
}

bool nrf_802154_delayed_trx_abort(nrf_802154_term_t term_lvl, req_originator_t req_orig)
{
    bool result = true;

    if (!dly_op_is_in_progress(RSCH_DLY_RX))
    {
        // No active procedures, just return true.
    }
    else if ((REQ_ORIG_HIGHER_LAYER == req_orig) || (term_lvl >= NRF_802154_TERM_802154))
    {
        nrf_802154_timer_sched_remove(&m_timeout_timer);
        dly_op_stop(RSCH_DLY_RX);
    }
    else
    {
        result = false;
    }

    return result;
}

void nrf_802154_delayed_trx_rx_started_hook(void)
{
    if (dly_op_is_in_progress(RSCH_DLY_RX))
    {
        if (nrf_802154_timer_sched_remaining_time_get(&m_timeout_timer)
            < nrf_802154_rx_duration_get(MAX_PACKET_SIZE, true))
        {
            m_timeout_timer.t0 = nrf_802154_timer_sched_time_get();
            m_timeout_timer.dt = nrf_802154_rx_duration_get(MAX_PACKET_SIZE, true);

            nrf_802154_timer_sched_add(&m_timeout_timer, true);
        }
    }
}

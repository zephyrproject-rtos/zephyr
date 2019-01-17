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
 *   This file implements Timer Coordinator module.
 *
 */

#include "nrf_802154_timer_coord.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_config.h"
#include "hal/nrf_ppi.h"
#include "platform/hp_timer/nrf_802154_hp_timer.h"
#include "platform/lp_timer/nrf_802154_lp_timer.h"

#define DIV_ROUND_POSITIVE(n, d) (((n) + (d) / 2) / (d))
#define DIV_ROUND_NEGATIVE(n, d) (((n) - (d) / 2) / (d))
#define DIV_ROUND(n, d)          ((((n) < 0) ^ ((d) < 0)) ?  \
                                  DIV_ROUND_NEGATIVE(n, d) : \
                                  DIV_ROUND_POSITIVE(n, d))

#define TIME_BASE                (1UL << 22)      ///< Unit used to calculate PPTB (Point per Time Base). It is not equal million to speed up computations and increase precision.
#define FIRST_RESYNC_TIME        TIME_BASE        ///< Delay of the first resynchronization. The first resynchronization is needed to measure timers drift.
#define RESYNC_TIME              (64 * TIME_BASE) ///< Delay of following resynchronizations.
#define EWMA_COEF                (8)              ///< Weight used in the EWMA algorithm.

#define PPI_CH0                  NRF_PPI_CHANNEL13
#define PPI_CH1                  NRF_PPI_CHANNEL14
#define PPI_CHGRP0               NRF_PPI_CHANNEL_GROUP1

#define PPI_SYNC                 PPI_CH0
#define PPI_TIMESTAMP            PPI_CH1
#define PPI_TIMESTAMP_GROUP      PPI_CHGRP0

#if NRF_802154_FRAME_TIMESTAMP_ENABLED
// Structure holding common timepoint from both timers.
typedef struct
{
    uint32_t lp_timer_time; ///< LP Timer time of common timepoint.
    uint32_t hp_timer_time; ///< HP Timer time of common timepoint.
} common_timepoint_t;

// Static variables.
static common_timepoint_t m_last_sync;    ///< Common timepoint of last synchronization event.
static volatile bool      m_synchronized; ///< If timers were synchronized since last start.
static bool               m_drift_known;  ///< If timer drift value is known.
static int32_t            m_drift;        ///< Drift of the HP timer relatively to the LP timer [PPTB].

void nrf_802154_timer_coord_init(void)
{
    uint32_t sync_event;
    uint32_t sync_task;

    m_drift       = 0;
    m_drift_known = 0;

    nrf_802154_hp_timer_init();

    sync_event = nrf_802154_lp_timer_sync_event_get();
    sync_task  = nrf_802154_hp_timer_sync_task_get();

    nrf_ppi_channel_endpoint_setup(PPI_SYNC, sync_event, sync_task);
    nrf_ppi_channel_enable(PPI_SYNC);

    nrf_ppi_channel_include_in_group(PPI_TIMESTAMP, PPI_TIMESTAMP_GROUP);
}

void nrf_802154_timer_coord_uninit(void)
{
    nrf_802154_hp_timer_deinit();

    nrf_ppi_channel_disable(PPI_SYNC);
    nrf_ppi_channel_endpoint_setup(PPI_SYNC, 0, 0);

    nrf_ppi_group_disable(PPI_TIMESTAMP_GROUP);
    nrf_ppi_channel_and_fork_endpoint_setup(PPI_TIMESTAMP, 0, 0, 0);
}

void nrf_802154_timer_coord_start(void)
{
    m_synchronized = false;
    nrf_802154_hp_timer_start();
    nrf_802154_hp_timer_sync_prepare();
    nrf_802154_lp_timer_sync_start_now();
}

void nrf_802154_timer_coord_stop(void)
{
    nrf_802154_hp_timer_stop();
    nrf_802154_lp_timer_sync_stop();
}

void nrf_802154_timer_coord_timestamp_prepare(uint32_t event_addr)
{
    nrf_ppi_channel_and_fork_endpoint_setup(PPI_TIMESTAMP,
                                            event_addr,
                                            nrf_802154_hp_timer_timestamp_task_get(),
                                            (uint32_t)nrf_ppi_task_group_disable_address_get(
                                                PPI_TIMESTAMP_GROUP));

    nrf_ppi_group_enable(PPI_TIMESTAMP_GROUP);
}

bool nrf_802154_timer_coord_timestamp_get(uint32_t * p_timestamp)
{
    uint32_t hp_timestamp;
    uint32_t hp_delta;
    int32_t  drift;

    assert(p_timestamp != NULL);

    if (!m_synchronized)
    {
        return false;
    }

    hp_timestamp = nrf_802154_hp_timer_timestamp_get();
    hp_delta     = hp_timestamp - m_last_sync.hp_timer_time;
    drift        = m_drift_known ?
                   (DIV_ROUND(((int64_t)m_drift * hp_delta), ((int64_t)TIME_BASE + m_drift))) : 0;
    *p_timestamp = m_last_sync.lp_timer_time + hp_delta - drift;

    return true;
}

void nrf_802154_lp_timer_synchronized(void)
{
    common_timepoint_t sync_time;
    uint32_t           lp_delta;
    uint32_t           hp_delta;
    int32_t            timers_diff;
    int32_t            drift;
    int32_t            tb_fraction_of_lp_delta;

    if (nrf_802154_hp_timer_sync_time_get(&sync_time.hp_timer_time))
    {
        sync_time.lp_timer_time = nrf_802154_lp_timer_sync_time_get();

        // Calculate timers drift
        if (m_synchronized)
        {
            lp_delta                = sync_time.lp_timer_time - m_last_sync.lp_timer_time;
            hp_delta                = sync_time.hp_timer_time - m_last_sync.hp_timer_time;
            tb_fraction_of_lp_delta = DIV_ROUND_POSITIVE(lp_delta, TIME_BASE);
            timers_diff             = hp_delta - lp_delta;
            drift                   = DIV_ROUND(timers_diff, tb_fraction_of_lp_delta); // Drift in PPTB

            if (m_drift_known)
            {
                m_drift = DIV_ROUND((m_drift * (EWMA_COEF - 1) + drift), EWMA_COEF);
            }
            else
            {
                m_drift = drift;
            }

            m_drift_known = true;
        }

        /* To avoid possible race when nrf_802154_timer_coord_timestamp_get
         * is called when m_last_sync is being assigned report that we are not synchronized
         * during assignment.
         * This is naive solution that can be improved if needed with double buffering.
         */
        m_synchronized = false;
        __DMB();
        m_last_sync = sync_time;
        __DMB();
        m_synchronized = true;

        nrf_802154_hp_timer_sync_prepare();
        nrf_802154_lp_timer_sync_start_at(m_last_sync.lp_timer_time,
                                          m_drift_known ? RESYNC_TIME : FIRST_RESYNC_TIME);
    }
    else
    {
        nrf_802154_hp_timer_sync_prepare();
        nrf_802154_lp_timer_sync_start_now();
    }
}

#else // NRF_802154_FRAME_TIMESTAMP_ENABLED

void nrf_802154_timer_coord_init(void)
{
    // Intentionally empty
}

void nrf_802154_timer_coord_uninit(void)
{
    // Intentionally empty
}

void nrf_802154_timer_coord_start(void)
{
    // Intentionally empty
}

void nrf_802154_timer_coord_stop(void)
{
    // Intentionally empty
}

void nrf_802154_timer_coord_timestamp_prepare(uint32_t event_addr)
{
    (void)event_addr;

    // Intentionally empty
}

bool nrf_802154_timer_coord_timestamp_get(uint32_t * p_timestamp)
{
    (void)p_timestamp;

    // Intentionally empty

    return false;
}

#endif // NRF_802154_FRAME_TIMESTAMP_ENABLED

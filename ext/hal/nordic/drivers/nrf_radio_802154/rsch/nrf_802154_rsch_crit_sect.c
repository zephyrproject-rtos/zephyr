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
 *   This file implements critical sections for the RSCH module.
 *
 */

#include "nrf_802154_rsch_crit_sect.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_critical_section.h"
#include "nrf_802154_rsch.h"

#include <nrf.h>

#define RSCH_EVT_NONE (rsch_prio_t)UINT8_MAX

static volatile uint8_t m_rsch_pending_evt; ///< Indicator of pending RSCH event.

/***************************************************************************************************
 * @section RSCH pending events management
 **************************************************************************************************/

static void rsch_pending_evt_set(rsch_prio_t prio)
{
    volatile uint8_t rsch_pending_evt; // Dummy variable to prevent compilers from removing ldrex

    do
    {
        rsch_pending_evt = __LDREXB(&m_rsch_pending_evt);
        (void)rsch_pending_evt;
    }
    while (__STREXB((uint8_t)prio, &m_rsch_pending_evt));
}

static rsch_prio_t rsch_pending_evt_clear(void)
{
    uint8_t evt_value;

    do
    {
        evt_value = __LDREXB(&m_rsch_pending_evt);
    }
    while (__STREXB(RSCH_EVT_NONE, &m_rsch_pending_evt));

    return (rsch_prio_t)evt_value;
}

static bool rsch_pending_evt_is_none(void)
{
    return m_rsch_pending_evt == (uint8_t)RSCH_EVT_NONE;
}

static void rsch_evt_process(rsch_prio_t evt)
{
    if (evt != RSCH_EVT_NONE)
    {
        nrf_802154_rsch_crit_sect_prio_changed(evt);
    }
}

/***************************************************************************************************
 * @section Public API
 **************************************************************************************************/

void nrf_802154_rsch_crit_sect_init(void)
{
    m_rsch_pending_evt = RSCH_EVT_NONE;
}

void nrf_802154_rsch_crit_sect_prio_request(rsch_prio_t prio)
{
    nrf_802154_rsch_continuous_mode_priority_set(prio);
}

/***************************************************************************************************
 * @section RSCH callbacks
 **************************************************************************************************/

void nrf_802154_rsch_continuous_prio_changed(rsch_prio_t prio)
{
    bool crit_sect_success = false;

    crit_sect_success = nrf_802154_critical_section_enter();

    // If we managed to enter critical section, but there is already a pending event,
    // it means that the Critical Section module is about to make one more iteration of the
    // critical section exit procedure. To prevent race in continuous mode priorities notification,
    // we do not notify directly, but just update the pending event.
    if (crit_sect_success && rsch_pending_evt_is_none())
    {
        nrf_802154_rsch_crit_sect_prio_changed(prio);
    }
    else
    {
        rsch_pending_evt_set(prio);
    }

    if (crit_sect_success)
    {
        nrf_802154_critical_section_exit();
    }
}

/***************************************************************************************************
 * @section Critical section callbacks
 **************************************************************************************************/

void nrf_802154_critical_section_rsch_enter(void)
{
    // Intentionally empty
}

void nrf_802154_critical_section_rsch_exit(void)
{
    rsch_evt_process(rsch_pending_evt_clear());
}

bool nrf_802154_critical_section_rsch_event_is_pending(void)
{
    return !rsch_pending_evt_is_none();
}

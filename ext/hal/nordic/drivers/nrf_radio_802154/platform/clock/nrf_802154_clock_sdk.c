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
 *   This file implements the nrf 802.15.4 HF Clock abstraction with SDK driver.
 *
 * This implementation uses clock driver implementation from SDK.
 */

#include "nrf_802154_clock.h"

#include <stddef.h>

#include <compiler_abstraction.h>
#include <nrf_drv_clock.h>

static void clock_handler(nrf_drv_clock_evt_type_t event);

static nrf_drv_clock_handler_item_t m_clock_handler =
{
    .p_next        = NULL,
    .event_handler = clock_handler,
};

static void clock_handler(nrf_drv_clock_evt_type_t event)
{
    if (event == NRF_DRV_CLOCK_EVT_HFCLK_STARTED)
    {
        nrf_802154_clock_hfclk_ready();
    }

    if (event == NRF_DRV_CLOCK_EVT_LFCLK_STARTED)
    {
        nrf_802154_clock_lfclk_ready();
    }
}

void nrf_802154_clock_init(void)
{
    nrf_drv_clock_init();
}

void nrf_802154_clock_deinit(void)
{
    nrf_drv_clock_uninit();
}

void nrf_802154_clock_hfclk_start(void)
{
    nrf_drv_clock_hfclk_request(&m_clock_handler);
}

void nrf_802154_clock_hfclk_stop(void)
{
    nrf_drv_clock_hfclk_release();
}

bool nrf_802154_clock_hfclk_is_running(void)
{
    return nrf_drv_clock_hfclk_is_running();
}

void nrf_802154_clock_lfclk_start(void)
{
    nrf_drv_clock_lfclk_request(&m_clock_handler);
}

void nrf_802154_clock_lfclk_stop(void)
{
    nrf_drv_clock_lfclk_release();
}

bool nrf_802154_clock_lfclk_is_running(void)
{
    return nrf_drv_clock_lfclk_is_running();
}

__WEAK void nrf_802154_clock_hfclk_ready(void)
{
    // Intentionally empty.
}

__WEAK void nrf_802154_clock_lfclk_ready(void)
{
    // Intentionally empty.
}

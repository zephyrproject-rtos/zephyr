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
 *   This file implements the nrf 802.15.4 Clock Abstraction Layer without any driver.
 *
 * This implementation uses directly CLOCK hardware registers.
 */

#include "nrf_802154_clock.h"

#include <nrf.h>
#include <hal/nrf_clock.h>

#include "nrf_802154_config.h"

void nrf_802154_clock_init(void)
{
    nrf_clock_lf_src_set(NRF_802154_CLOCK_LFCLK_SOURCE);

    NVIC_SetPriority(POWER_CLOCK_IRQn, NRF_802154_CLOCK_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);
    NVIC_EnableIRQ(POWER_CLOCK_IRQn);

    nrf_clock_int_enable(NRF_CLOCK_INT_HF_STARTED_MASK);
    nrf_clock_int_enable(NRF_CLOCK_INT_LF_STARTED_MASK);
}

void nrf_802154_clock_deinit(void)
{
    NVIC_DisableIRQ(POWER_CLOCK_IRQn);
    NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);

    nrf_clock_int_disable(NRF_CLOCK_INT_HF_STARTED_MASK);
    nrf_clock_int_disable(NRF_CLOCK_INT_LF_STARTED_MASK);
}

void nrf_802154_clock_hfclk_start(void)
{
    nrf_clock_event_clear(NRF_CLOCK_EVENT_HFCLKSTARTED);
    nrf_clock_task_trigger(NRF_CLOCK_TASK_HFCLKSTART);
}

void nrf_802154_clock_hfclk_stop(void)
{
    nrf_clock_task_trigger(NRF_CLOCK_TASK_HFCLKSTOP);
}

bool nrf_802154_clock_hfclk_is_running(void)
{
    return nrf_clock_hf_is_running(NRF_CLOCK_HFCLK_HIGH_ACCURACY);
}

void nrf_802154_clock_lfclk_start(void)
{
    nrf_clock_event_clear(NRF_CLOCK_EVENT_LFCLKSTARTED);
    nrf_clock_task_trigger(NRF_CLOCK_TASK_LFCLKSTART);
}

void nrf_802154_clock_lfclk_stop(void)
{
    nrf_clock_task_trigger(NRF_CLOCK_TASK_LFCLKSTOP);
}

bool nrf_802154_clock_lfclk_is_running(void)
{
    return nrf_clock_lf_is_running();
}

void POWER_CLOCK_IRQHandler(void)
{
    if (nrf_clock_event_check(NRF_CLOCK_EVENT_HFCLKSTARTED))
    {
        nrf_clock_event_clear(NRF_CLOCK_EVENT_HFCLKSTARTED);

        nrf_802154_clock_hfclk_ready();
    }

    if (nrf_clock_event_check(NRF_CLOCK_EVENT_LFCLKSTARTED))
    {
        nrf_clock_event_clear(NRF_CLOCK_EVENT_LFCLKSTARTED);

        nrf_802154_clock_lfclk_ready();
    }
}

__WEAK void nrf_802154_clock_hfclk_ready(void)
{
    // Intentionally empty.
}

__WEAK void nrf_802154_clock_lfclk_ready(void)
{
    // Intentionally empty.
}

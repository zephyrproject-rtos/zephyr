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
 *   This file implements requests to the driver triggered directly by the MAC layer.
 *
 */

#include "nrf_802154_request.h"

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_core.h"
#include "hal/nrf_radio.h"

#define REQUEST_FUNCTION(func_core, ...) \
    bool result;                         \
                                         \
    result = func_core(__VA_ARGS__);     \
                                         \
    return result;

void nrf_802154_request_init(void)
{
    // Intentionally empty
}

bool nrf_802154_request_sleep(nrf_802154_term_t term_lvl)
{
    REQUEST_FUNCTION(nrf_802154_core_sleep, term_lvl)
}

bool nrf_802154_request_receive(nrf_802154_term_t              term_lvl,
                                req_originator_t               req_orig,
                                nrf_802154_notification_func_t notify_function,
                                bool                           notify_abort)
{
    REQUEST_FUNCTION(nrf_802154_core_receive, term_lvl, req_orig, notify_function, notify_abort)
}

bool nrf_802154_request_transmit(nrf_802154_term_t              term_lvl,
                                 req_originator_t               req_orig,
                                 const uint8_t                * p_data,
                                 bool                           cca,
                                 bool                           immediate,
                                 nrf_802154_notification_func_t notify_function)
{
    REQUEST_FUNCTION(nrf_802154_core_transmit,
                     term_lvl,
                     req_orig,
                     p_data,
                     cca,
                     immediate,
                     notify_function)
}

bool nrf_802154_request_energy_detection(nrf_802154_term_t term_lvl, uint32_t time_us)
{
    REQUEST_FUNCTION(nrf_802154_core_energy_detection, term_lvl, time_us)
}

bool nrf_802154_request_cca(nrf_802154_term_t term_lvl)
{
    REQUEST_FUNCTION(nrf_802154_core_cca, term_lvl)
}

bool nrf_802154_request_continuous_carrier(nrf_802154_term_t term_lvl)
{
    REQUEST_FUNCTION(nrf_802154_core_continuous_carrier, term_lvl)
}

bool nrf_802154_request_buffer_free(uint8_t * p_data)
{
    REQUEST_FUNCTION(nrf_802154_core_notify_buffer_free, p_data)
}

bool nrf_802154_request_channel_update(void)
{
    REQUEST_FUNCTION(nrf_802154_core_channel_update)
}

bool nrf_802154_request_cca_cfg_update(void)
{
    REQUEST_FUNCTION(nrf_802154_core_cca_cfg_update)
}

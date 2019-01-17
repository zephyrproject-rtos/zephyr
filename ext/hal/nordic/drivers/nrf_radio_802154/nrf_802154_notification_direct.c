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
 *   This file implements notifications triggered directly by the nrf 802.15.4 radio driver.
 *
 */

#include "nrf_802154_notification.h"

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154.h"
#include "nrf_802154_critical_section.h"

#define RAW_LENGTH_OFFSET  0
#define RAW_PAYLOAD_OFFSET 1

void nrf_802154_notification_init(void)
{
    // Intentionally empty
}

void nrf_802154_notify_received(uint8_t * p_data, int8_t power, int8_t lqi)
{
#if NRF_802154_USE_RAW_API
    nrf_802154_received_raw(p_data, power, lqi);
#else // NRF_802154_USE_RAW_API
    nrf_802154_received(p_data + RAW_PAYLOAD_OFFSET, p_data[RAW_LENGTH_OFFSET], power, lqi);
#endif  // NRF_802154_USE_RAW_API
}

void nrf_802154_notify_receive_failed(nrf_802154_rx_error_t error)
{
    nrf_802154_receive_failed(error);
}

void nrf_802154_notify_transmitted(const uint8_t * p_frame,
                                   uint8_t       * p_ack,
                                   int8_t          power,
                                   int8_t          lqi)
{
#if NRF_802154_USE_RAW_API
    nrf_802154_transmitted_raw(p_frame, p_ack, power, lqi);
#else // NRF_802154_USE_RAW_API
    nrf_802154_transmitted(p_frame + RAW_PAYLOAD_OFFSET,
                           p_ack == NULL ? NULL : p_ack + RAW_PAYLOAD_OFFSET,
                           p_ack[RAW_LENGTH_OFFSET],
                           power,
                           lqi);
#endif // NRF_802154_USE_RAW_API
}

void nrf_802154_notify_transmit_failed(const uint8_t * p_frame, nrf_802154_tx_error_t error)
{
#if NRF_802154_USE_RAW_API
    nrf_802154_transmit_failed(p_frame, error);
#else // NRF_802154_USE_RAW_API
    nrf_802154_transmit_failed(p_frame + RAW_PAYLOAD_OFFSET, error);
#endif  // NRF_802154_USE_RAW_API
}

void nrf_802154_notify_energy_detected(uint8_t result)
{
    nrf_802154_energy_detected(result);
}

void nrf_802154_notify_energy_detection_failed(nrf_802154_ed_error_t error)
{
    nrf_802154_energy_detection_failed(error);
}

void nrf_802154_notify_cca(bool is_free)
{
    nrf_802154_cca_done(is_free);
}

void nrf_802154_notify_cca_failed(nrf_802154_cca_error_t error)
{
    nrf_802154_cca_failed(error);
}

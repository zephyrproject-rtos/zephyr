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
 *   This file implements the nrf 802.15.4 radio driver.
 *
 */

#include "nrf_802154.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "nrf_802154_ack_pending_bit.h"
#include "nrf_802154_config.h"
#include "nrf_802154_const.h"
#include "nrf_802154_core.h"
#include "nrf_802154_critical_section.h"
#include "nrf_802154_debug.h"
#include "nrf_802154_notification.h"
#include "nrf_802154_pib.h"
#include "nrf_802154_priority_drop.h"
#include "nrf_802154_request.h"
#include "nrf_802154_revision.h"
#include "nrf_802154_rsch.h"
#include "nrf_802154_rsch_crit_sect.h"
#include "nrf_802154_rssi.h"
#include "nrf_802154_rx_buffer.h"
#include "nrf_802154_timer_coord.h"
#include "hal/nrf_radio.h"
#include "platform/clock/nrf_802154_clock.h"
#include "platform/lp_timer/nrf_802154_lp_timer.h"
#include "platform/temperature/nrf_802154_temperature.h"
#include "timer_scheduler/nrf_802154_timer_sched.h"

#include "mac_features/nrf_802154_ack_timeout.h"
#include "mac_features/nrf_802154_csma_ca.h"
#include "mac_features/nrf_802154_delayed_trx.h"

#if ENABLE_FEM
#include "fem/nrf_fem_control_api.h"
#endif

#define RAW_LENGTH_OFFSET  0
#define RAW_PAYLOAD_OFFSET 1

#if !NRF_802154_USE_RAW_API
/** Static transmit buffer used by @sa nrf_802154_transmit() family of functions.
 *
 * If none of functions using this buffer is called and link time optimization is enabled, this
 * buffer should be removed by linker.
 */
static uint8_t m_tx_buffer[RAW_PAYLOAD_OFFSET + MAX_PACKET_SIZE];

/**
 * @brief Fill transmit buffer with given data.
 *
 * @param[in]  p_data   Pointer to array containing payload of a data to transmit. The array
 *                      should exclude PHR or FCS fields of 802.15.4 frame.
 * @param[in]  length   Length of given frame. This value shall exclude PHR and FCS fields from
 *                      the given frame (exact size of buffer pointed by @p p_data).
 */
static void tx_buffer_fill(const uint8_t * p_data, uint8_t length)
{
    assert(length <= MAX_PACKET_SIZE - FCS_SIZE);

    m_tx_buffer[RAW_LENGTH_OFFSET] = length + FCS_SIZE;
    memcpy(&m_tx_buffer[RAW_PAYLOAD_OFFSET], p_data, length);
}

#endif // !NRF_802154_USE_RAW_API

/**
 * @brief Get timestamp of the last received frame.
 *
 * @note This function increments the returned value by 1 us if the timestamp is equal to the
 *       @ref NRF_802154_NO_TIMESTAMP value to indicate that the timestamp is available.
 *
 * @returns Timestamp [us] of the last received frame or @ref NRF_802154_NO_TIMESTAMP if
 *          the timestamp is inaccurate.
 */
static uint32_t last_rx_frame_timestamp_get(void)
{
#if NRF_802154_FRAME_TIMESTAMP_ENABLED
    uint32_t timestamp;
    bool     timestamp_received = nrf_802154_timer_coord_timestamp_get(&timestamp);

    if (!timestamp_received)
    {
        timestamp = NRF_802154_NO_TIMESTAMP;
    }
    else
    {
        if (timestamp == NRF_802154_NO_TIMESTAMP)
        {
            timestamp++;
        }
    }

    return timestamp;
#else // NRF_802154_FRAME_TIMESTAMP_ENABLED
    return NRF_802154_NO_TIMESTAMP;
#endif  // NRF_802154_FRAME_TIMESTAMP_ENABLED
}

void nrf_802154_channel_set(uint8_t channel)
{
    bool changed = nrf_802154_pib_channel_get() != channel;

    nrf_802154_pib_channel_set(channel);

    if (changed)
    {
        nrf_802154_request_channel_update();
    }
}

uint8_t nrf_802154_channel_get(void)
{
    return nrf_802154_pib_channel_get();
}

void nrf_802154_tx_power_set(int8_t power)
{
    nrf_802154_pib_tx_power_set(power);
}

int8_t nrf_802154_tx_power_get(void)
{
    return nrf_802154_pib_tx_power_get();
}

void nrf_802154_temperature_changed(void)
{
    nrf_802154_request_cca_cfg_update();
}

void nrf_802154_pan_id_set(const uint8_t * p_pan_id)
{
    nrf_802154_pib_pan_id_set(p_pan_id);
}

void nrf_802154_extended_address_set(const uint8_t * p_extended_address)
{
    nrf_802154_pib_extended_address_set(p_extended_address);
}

void nrf_802154_short_address_set(const uint8_t * p_short_address)
{
    nrf_802154_pib_short_address_set(p_short_address);
}

int8_t nrf_802154_dbm_from_energy_level_calculate(uint8_t energy_level)
{
    return ED_MIN_DBM + (energy_level / ED_RESULT_FACTOR);
}

uint8_t nrf_802154_ccaedthres_from_dbm_calculate(int8_t dbm)
{
    return dbm - ED_MIN_DBM;
}

uint32_t nrf_802154_first_symbol_timestamp_get(uint32_t end_timestamp, uint8_t psdu_length)
{
    uint32_t frame_symbols = PHY_SHR_SYMBOLS;

    frame_symbols += (PHR_SIZE + psdu_length) * PHY_SYMBOLS_PER_OCTET;

    return end_timestamp - (frame_symbols * PHY_US_PER_SYMBOL);
}

void nrf_802154_init(void)
{
    nrf_802154_ack_pending_bit_init();
    nrf_802154_core_init();
    nrf_802154_clock_init();
    nrf_802154_critical_section_init();
    nrf_802154_debug_init();
    nrf_802154_notification_init();
    nrf_802154_lp_timer_init();
    nrf_802154_pib_init();
    nrf_802154_priority_drop_init();
    nrf_802154_request_init();
    nrf_802154_revision_init();
    nrf_802154_rsch_crit_sect_init();
    nrf_802154_rsch_init();
    nrf_802154_rx_buffer_init();
    nrf_802154_temperature_init();
    nrf_802154_timer_coord_init();
    nrf_802154_timer_sched_init();
}

void nrf_802154_deinit(void)
{
    nrf_802154_timer_sched_deinit();
    nrf_802154_timer_coord_uninit();
    nrf_802154_temperature_deinit();
    nrf_802154_rsch_uninit();
    nrf_802154_lp_timer_deinit();
    nrf_802154_clock_deinit();
    nrf_802154_core_deinit();
}

#if !NRF_802154_INTERNAL_RADIO_IRQ_HANDLING
void nrf_802154_radio_irq_handler(void)
{
    nrf_802154_core_irq_handler();
}

#endif // !NRF_802154_INTERNAL_RADIO_IRQ_HANDLING

#if ENABLE_FEM
void nrf_802154_fem_control_cfg_set(const nrf_802154_fem_control_cfg_t * p_cfg)
{
    nrf_fem_control_cfg_set(p_cfg);
}

void nrf_802154_fem_control_cfg_get(nrf_802154_fem_control_cfg_t * p_cfg)
{
    nrf_fem_control_cfg_get(p_cfg);
}

#endif // ENABLE_FEM

nrf_802154_state_t nrf_802154_state_get(void)
{
    switch (nrf_802154_core_state_get())
    {
        case RADIO_STATE_SLEEP:
        case RADIO_STATE_FALLING_ASLEEP:
            return NRF_802154_STATE_SLEEP;

        case RADIO_STATE_RX:
        case RADIO_STATE_TX_ACK:
            return NRF_802154_STATE_RECEIVE;

        case RADIO_STATE_CCA_TX:
        case RADIO_STATE_TX:
        case RADIO_STATE_RX_ACK:
            return NRF_802154_STATE_TRANSMIT;

        case RADIO_STATE_ED:
            return NRF_802154_STATE_ENERGY_DETECTION;

        case RADIO_STATE_CCA:
            return NRF_802154_STATE_CCA;

        case RADIO_STATE_CONTINUOUS_CARRIER:
            return NRF_802154_STATE_CONTINUOUS_CARRIER;
    }

    return NRF_802154_STATE_INVALID;
}

bool nrf_802154_sleep(void)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_SLEEP);

    result = nrf_802154_request_sleep(NRF_802154_TERM_802154);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_SLEEP);
    return result;
}

nrf_802154_sleep_error_t nrf_802154_sleep_if_idle(void)
{
    nrf_802154_sleep_error_t result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_SLEEP);

    result =
        nrf_802154_request_sleep(NRF_802154_TERM_NONE) ? NRF_802154_SLEEP_ERROR_NONE :
        NRF_802154_SLEEP_ERROR_BUSY;

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_SLEEP);
    return result;
}

bool nrf_802154_receive(void)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RECEIVE);

    result = nrf_802154_request_receive(NRF_802154_TERM_802154, REQ_ORIG_HIGHER_LAYER, NULL, true);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RECEIVE);
    return result;
}

#if NRF_802154_USE_RAW_API
bool nrf_802154_transmit_raw(const uint8_t * p_data, bool cca)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_TRANSMIT);

    result = nrf_802154_request_transmit(NRF_802154_TERM_NONE,
                                         REQ_ORIG_HIGHER_LAYER,
                                         p_data,
                                         cca,
                                         false,
                                         NULL);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_TRANSMIT);
    return result;
}

#else // NRF_802154_USE_RAW_API

bool nrf_802154_transmit(const uint8_t * p_data, uint8_t length, bool cca)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_TRANSMIT);

    tx_buffer_fill(p_data, length);
    result = nrf_802154_request_transmit(NRF_802154_TERM_NONE,
                                         REQ_ORIG_HIGHER_LAYER,
                                         m_tx_buffer,
                                         cca,
                                         false,
                                         NULL);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_TRANSMIT);
    return result;
}

#endif // NRF_802154_USE_RAW_API

bool nrf_802154_transmit_raw_at(const uint8_t * p_data,
                                bool            cca,
                                uint32_t        t0,
                                uint32_t        dt,
                                uint8_t         channel)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_TRANSMIT_AT);

    result = nrf_802154_delayed_trx_transmit(p_data, cca, t0, dt, channel);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_TRANSMIT_AT);
    return result;
}

bool nrf_802154_receive_at(uint32_t t0,
                           uint32_t dt,
                           uint32_t timeout,
                           uint8_t  channel)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RECEIVE_AT);

    result = nrf_802154_delayed_trx_receive(t0, dt, timeout, channel);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RECEIVE_AT);
    return result;
}

bool nrf_802154_energy_detection(uint32_t time_us)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_ENERGY_DETECTION);

    result = nrf_802154_request_energy_detection(NRF_802154_TERM_NONE, time_us);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_ENERGY_DETECTION);
    return result;
}

bool nrf_802154_cca(void)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_CCA);

    result = nrf_802154_request_cca(NRF_802154_TERM_NONE);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_CCA);
    return result;
}

bool nrf_802154_continuous_carrier(void)
{
    bool result;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_CONTINUOUS_CARRIER);

    result = nrf_802154_request_continuous_carrier(NRF_802154_TERM_NONE);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_CONTINUOUS_CARRIER);
    return result;
}

#if NRF_802154_USE_RAW_API

void nrf_802154_buffer_free_raw(uint8_t * p_data)
{
    bool          result;
    rx_buffer_t * p_buffer = (rx_buffer_t *)p_data;

    assert(p_buffer->free == false);
    (void)p_buffer;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_BUFFER_FREE);

    result = nrf_802154_request_buffer_free(p_data);
    assert(result);
    (void)result;

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_BUFFER_FREE);
}

bool nrf_802154_buffer_free_immediately_raw(uint8_t * p_data)
{
    bool          result;
    rx_buffer_t * p_buffer = (rx_buffer_t *)p_data;

    assert(p_buffer->free == false);
    (void)p_buffer;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_BUFFER_FREE);

    result = nrf_802154_request_buffer_free(p_data);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_BUFFER_FREE);
    return result;
}

#else // NRF_802154_USE_RAW_API

void nrf_802154_buffer_free(uint8_t * p_data)
{
    bool          result;
    rx_buffer_t * p_buffer = (rx_buffer_t *)(p_data - RAW_PAYLOAD_OFFSET);

    assert(p_buffer->free == false);
    (void)p_buffer;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_BUFFER_FREE);

    result = nrf_802154_request_buffer_free(p_data - RAW_PAYLOAD_OFFSET);
    assert(result);
    (void)result;

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_BUFFER_FREE);
}

bool nrf_802154_buffer_free_immediately(uint8_t * p_data)
{
    bool          result;
    rx_buffer_t * p_buffer = (rx_buffer_t *)(p_data - RAW_PAYLOAD_OFFSET);

    assert(p_buffer->free == false);
    (void)p_buffer;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_BUFFER_FREE);

    result = nrf_802154_request_buffer_free(p_data - RAW_PAYLOAD_OFFSET);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_BUFFER_FREE);
    return result;
}

#endif // NRF_802154_USE_RAW_API

int8_t nrf_802154_rssi_last_get(void)
{
    uint8_t negative_dbm = nrf_radio_rssi_sample_get();

    negative_dbm = nrf_802154_rssi_sample_corrected_get(negative_dbm);

    return -(int8_t)negative_dbm;
}

bool nrf_802154_promiscuous_get(void)
{
    return nrf_802154_pib_promiscuous_get();
}

void nrf_802154_promiscuous_set(bool enabled)
{
    nrf_802154_pib_promiscuous_set(enabled);
}

void nrf_802154_auto_ack_set(bool enabled)
{
    nrf_802154_pib_auto_ack_set(enabled);
}

bool nrf_802154_auto_ack_get(void)
{
    return nrf_802154_pib_auto_ack_get();
}

bool nrf_802154_pan_coord_get(void)
{
    return nrf_802154_pib_pan_coord_get();
}

void nrf_802154_pan_coord_set(bool enabled)
{
    nrf_802154_pib_pan_coord_set(enabled);
}

void nrf_802154_auto_pending_bit_set(bool enabled)
{
    nrf_802154_ack_pending_bit_set(enabled);
}

bool nrf_802154_pending_bit_for_addr_set(const uint8_t * p_addr, bool extended)
{
    return nrf_802154_ack_pending_bit_for_addr_set(p_addr, extended);
}

bool nrf_802154_pending_bit_for_addr_clear(const uint8_t * p_addr, bool extended)
{
    return nrf_802154_ack_pending_bit_for_addr_clear(p_addr, extended);
}

void nrf_802154_pending_bit_for_addr_reset(bool extended)
{
    nrf_802154_ack_pending_bit_for_addr_reset(extended);
}

void nrf_802154_cca_cfg_set(const nrf_802154_cca_cfg_t * p_cca_cfg)
{
    nrf_802154_pib_cca_cfg_set(p_cca_cfg);

    nrf_802154_request_cca_cfg_update();
}

void nrf_802154_cca_cfg_get(nrf_802154_cca_cfg_t * p_cca_cfg)
{
    nrf_802154_pib_cca_cfg_get(p_cca_cfg);
}

#if NRF_802154_CSMA_CA_ENABLED
#if NRF_802154_USE_RAW_API

void nrf_802154_transmit_csma_ca_raw(const uint8_t * p_data)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_CSMACA);

    nrf_802154_csma_ca_start(p_data);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_CSMACA);
}

#else // NRF_802154_USE_RAW_API

void nrf_802154_transmit_csma_ca(const uint8_t * p_data, uint8_t length)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_CSMACA);

    tx_buffer_fill(p_data, length);

    nrf_802154_csma_ca_start(m_tx_buffer);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_CSMACA);
}

#endif // NRF_802154_USE_RAW_API
#endif // NRF_802154_CSMA_CA_ENABLED

#if NRF_802154_ACK_TIMEOUT_ENABLED

void nrf_802154_ack_timeout_set(uint32_t time)
{
    nrf_802154_ack_timeout_time_set(time);
}

#endif // NRF_802154_ACK_TIMEOUT_ENABLED

__WEAK void nrf_802154_tx_ack_started(void)
{
    // Intentionally empty
}

#if NRF_802154_USE_RAW_API
__WEAK void nrf_802154_received_raw(uint8_t * p_data, int8_t power, uint8_t lqi)
{
    nrf_802154_received_timestamp_raw(p_data, power, lqi, last_rx_frame_timestamp_get());
}

__WEAK void nrf_802154_received_timestamp_raw(uint8_t * p_data,
                                              int8_t    power,
                                              uint8_t   lqi,
                                              uint32_t  time)
{
    (void)power;
    (void)lqi;
    (void)time;

    nrf_802154_buffer_free_raw(p_data);
}

#else // NRF_802154_USE_RAW_API

__WEAK void nrf_802154_received(uint8_t * p_data, uint8_t length, int8_t power, uint8_t lqi)
{
    nrf_802154_received_timestamp(p_data, length, power, lqi, last_rx_frame_timestamp_get());
}

__WEAK void nrf_802154_received_timestamp(uint8_t * p_data,
                                          uint8_t   length,
                                          int8_t    power,
                                          uint8_t   lqi,
                                          uint32_t  time)
{
    (void)length;
    (void)power;
    (void)lqi;
    (void)time;

    nrf_802154_buffer_free(p_data);
}

#endif // !NRF_802154_USE_RAW_API

__WEAK void nrf_802154_receive_failed(nrf_802154_rx_error_t error)
{
    (void)error;
}

__WEAK void nrf_802154_tx_started(const uint8_t * p_frame)
{
    (void)p_frame;
}

#if NRF_802154_USE_RAW_API
__WEAK void nrf_802154_transmitted_raw(const uint8_t * p_frame,
                                       uint8_t       * p_ack,
                                       int8_t          power,
                                       uint8_t         lqi)
{
    uint32_t timestamp = (p_ack == NULL) ? NRF_802154_NO_TIMESTAMP : last_rx_frame_timestamp_get();

    nrf_802154_transmitted_timestamp_raw(p_frame, p_ack, power, lqi, timestamp);
}

__WEAK void nrf_802154_transmitted_timestamp_raw(const uint8_t * p_frame,
                                                 uint8_t       * p_ack,
                                                 int8_t          power,
                                                 uint8_t         lqi,
                                                 uint32_t        time)
{
    (void)p_frame;
    (void)power;
    (void)lqi;
    (void)time;

    if (p_ack != NULL)
    {
        nrf_802154_buffer_free_raw(p_ack);
    }
}

#else // NRF_802154_USE_RAW_API

__WEAK void nrf_802154_transmitted(const uint8_t * p_frame,
                                   uint8_t       * p_ack,
                                   uint8_t         length,
                                   int8_t          power,
                                   uint8_t         lqi)
{
    uint32_t timestamp = (p_ack == NULL) ? NRF_802154_NO_TIMESTAMP : last_rx_frame_timestamp_get();

    nrf_802154_transmitted_timestamp(p_frame, p_ack, length, power, lqi, timestamp);
}

__WEAK void nrf_802154_transmitted_timestamp(const uint8_t * p_frame,
                                             uint8_t       * p_ack,
                                             uint8_t         length,
                                             int8_t          power,
                                             uint8_t         lqi,
                                             uint32_t        time)
{
    (void)p_frame;
    (void)length;
    (void)power;
    (void)lqi;
    (void)time;

    if (p_ack != NULL)
    {
        nrf_802154_buffer_free(p_ack);
    }
}

#endif // NRF_802154_USE_RAW_API

__WEAK void nrf_802154_transmit_failed(const uint8_t * p_frame, nrf_802154_tx_error_t error)
{
    (void)p_frame;
    (void)error;
}

__WEAK void nrf_802154_energy_detected(uint8_t result)
{
    (void)result;
}

__WEAK void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
    (void)error;
}

__WEAK void nrf_802154_cca_done(bool channel_free)
{
    (void)channel_free;
}

__WEAK void nrf_802154_cca_failed(nrf_802154_cca_error_t error)
{
    (void)error;
}

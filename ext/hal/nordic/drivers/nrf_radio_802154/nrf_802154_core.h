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
 * @brief This module contains core of the nRF IEEE 802.15.4 radio driver.
 *
 */

#ifndef NRF_802154_CORE_H_
#define NRF_802154_CORE_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_config.h"
#include "nrf_802154_notification.h"
#include "nrf_802154_rx_buffer.h"
#include "nrf_802154_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief States of nRF 802.15.4 driver.
 */
typedef enum
{
    // Sleep
    RADIO_STATE_SLEEP,              ///< Low power (DISABLED) mode - the only state in which all radio preconditions ane not requested.
    RADIO_STATE_FALLING_ASLEEP,     ///< Prior entering SLEEP state all radio preconditions are requested.

    // Receive
    RADIO_STATE_RX,                 ///< Receiver is enabled and it is receiving frames.
    RADIO_STATE_TX_ACK,             ///< Received frame and transmitting ACK.

    // Transmit
    RADIO_STATE_CCA_TX,             ///< Performing CCA followed by frame transmission.
    RADIO_STATE_TX,                 ///< Transmitting data frame (or beacon).
    RADIO_STATE_RX_ACK,             ///< Receiving ACK after transmitted frame.

    // Energy Detection
    RADIO_STATE_ED,                 ///< Performing Energy Detection procedure.

    // CCA
    RADIO_STATE_CCA,                ///< Performing CCA procedure.

    // Continuous carrier
    RADIO_STATE_CONTINUOUS_CARRIER, ///< Emitting continuous carrier wave.
} radio_state_t;

/**
 * @brief Initialize 802.15.4 driver core.
 */
void nrf_802154_core_init(void);

/**
 * @brief Deinitialize 802.15.4 driver core.
 */
void nrf_802154_core_deinit(void);

/**
 * @brief Get current state of nRF 802.15.4 driver.
 *
 * @return  Current state of the 802.15.4 driver.
 */
radio_state_t nrf_802154_core_state_get(void);

/***************************************************************************************************
 * @section State machine transition requests
 **************************************************************************************************/

/**
 * @brief Request transition to SLEEP state.
 *
 * @note This function shall be called from a critical section context. It shall not be interrupted
 *       by the RADIO event handler or Radio Shceduler notification.
 *
 * @param[in]  term_lvl  Termination level of this request. Selects procedures to abort.
 *
 * @retval  true   Entering SLEEP state succeeded.
 * @retval  false  Entering SLEEP state failed (driver is performing other procedure).
 */
bool nrf_802154_core_sleep(nrf_802154_term_t term_lvl);

/**
 * @brief Request transition to RECEIVE state.
 *
 * @note This function shall be called from a critical section context. It shall not be interrupted
 *       by the RADIO event handler or Radio Scheduler notification.
 *
 * @param[in]  term_lvl         Termination level of this request. Selects procedures to abort.
 * @param[in]  req_orig         Module that originates this request.
 * @param[in]  notify_function  Function called to notify status of this procedure. May be NULL.
 * @param[in]  notify_abort     If abort notification should be triggered.
 *
 * @retval  true   Entering RECEIVE state succeeded.
 * @retval  false  Entering RECEIVE state failed (driver is performing other procedure).
 */
bool nrf_802154_core_receive(nrf_802154_term_t              term_lvl,
                             req_originator_t               req_orig,
                             nrf_802154_notification_func_t notify_function,
                             bool                           notify_abort);

/**
 * @brief Request transition to TRANSMIT state.
 *
 * @note This function shall be called from a critical section context. It shall not be interrupted
 *       by the RADIO event handler or Radio Scheduler notification.
 *
 * @param[in]  term_lvl         Termination level of this request. Selects procedures to abort.
 * @param[in]  req_orig         Module that originates this request.
 * @param[in]  p_data           Pointer to a frame to transmit.
 * @param[in]  cca              If the driver should perform CCA procedure before transmission.
 * @param[in]  immediate        If true, the driver schedules transmission immediately or never;
 *                              if false transmission may be postponed until tx preconditions are
 *                              met.
 * @param[in]  notify_function  Function called to notify status of this procedure. May be NULL.
 *
 * @retval  true   Entering TRANSMIT state succeeded.
 * @retval  false  Entering TRANSMIT state failed (driver is performing other procedure).
 */
bool nrf_802154_core_transmit(nrf_802154_term_t              term_lvl,
                              req_originator_t               req_orig,
                              const uint8_t                * p_data,
                              bool                           cca,
                              bool                           immediate,
                              nrf_802154_notification_func_t notify_function);

/**
 * @brief Request transition to ENERGY_DETECTION state.
 *
 * @note This function shall be called from a critical section context. It shall not be interrupted
 *       by the RADIO event handler or Radio Scheduler notification.
 *
 * @note This function shall be called when the driver is in SLEEP or RECEIVE state. When Energy
 *       detection procedure is finished the driver will transit to RECEIVE state.
 *
 * @param[in]  term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[in]  time_us   Minimal time of energy detection procedure.
 *
 * @retval  true   Entering ENERGY_DETECTION state succeeded.
 * @retval  false  Entering ENERGY_DETECTION state failed (driver is performing other procedure).
 */
bool nrf_802154_core_energy_detection(nrf_802154_term_t term_lvl, uint32_t time_us);

/**
 * @brief Request transition to CCA state.
 *
 * @note This function shall be called from a critical section context. It shall not be interrupted
 *       by the RADIO event handler or Radio Scheduler notification.
 *
 * @param[in]  term_lvl  Termination level of this request. Selects procedures to abort.
 *
 * @retval  true   Entering CCA state succeeded.
 * @retval  false  Entering CCA state failed (driver is performing other procedure).
 */
bool nrf_802154_core_cca(nrf_802154_term_t term_lvl);

/**
 * @brief Request transition to CONTINUOUS_CARRIER state.
 *
 * @note This function shall be called from a critical section context. It shall not be interrupted
 *       by the RADIO event handler or Radio Scheduler notification.
 *
 * @param[in]  term_lvl  Termination level of this request. Selects procedures to abort.
 *
 * @retval  true   Entering CONTINUOUS_CARRIER state succeeded.
 * @retval  false  Entering CONTINUOUS_CARRIER state failed (driver is performing other procedure).
 */
bool nrf_802154_core_continuous_carrier(nrf_802154_term_t term_lvl);

/***************************************************************************************************
 * @section State machine notifications
 **************************************************************************************************/

/**
 * @brief Notify the Core module that higher layer freed a frame buffer.
 *
 * When there were no free buffers available the core does not start receiver. If core receives this
 * notification it changes internal state to make sure receiver is started if requested.
 *
 * @note This function shall be called from a critical section context. It shall not be interrupted
 *       by the RADIO event handler or Radio Scheduler notification.
 *
 * @param[in]  p_data  Pointer to buffer that has been freed.
 */
bool nrf_802154_core_notify_buffer_free(uint8_t * p_data);

/**
 * @brief Notify the Core module that next higher layer requested change of the channel.
 *
 * Core should update frequency register of the peripheral and in case it is in RECEIVE state the
 * receiver should be disabled and enabled again to use new channel.
 */
bool nrf_802154_core_channel_update(void);

/**
 * @brief Notify the Core module that next higher layer requested change of the CCA configuration.
 */
bool nrf_802154_core_cca_cfg_update(void);

#if !NRF_802154_INTERNAL_IRQ_HANDLING
/**
 * @brief Notify the Core module that there is a pending IRQ that should be handled.
 */
void nrf_802154_core_irq_handler(void);
#endif // !NRF_802154_INTERNAL_IRQ_HANDLING

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_CORE_H_ */

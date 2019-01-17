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

#ifndef NRF_802154_SWI_H__
#define NRF_802154_SWI_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154.h"
#include "nrf_802154_const.h"
#include "nrf_802154_notification.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_swi 802.15.4 driver SWI management
 * @{
 * @ingroup nrf_802154
 * @brief SWI manager for 802.15.4 driver.
 */

/**
 * @brief Initialize SWI module.
 */
void nrf_802154_swi_init(void);

/**
 * @brief Notify next higher layer that a frame was received from SWI priority level.
 *
 * @param[in]  p_data  Array of bytes containing PSDU. First byte contains frame length, other contain the frame itself.
 * @param[in]  power   RSSI measured during the frame reception.
 * @param[in]  lqi     LQI indicating measured link quality during the frame reception.
 */
void nrf_802154_swi_notify_received(uint8_t * p_data, int8_t power, int8_t lqi);

/**
 * @brief Notify next higher layer that reception of a frame failed.
 *
 * @param[in]  error  An error code that indicates reason of the failure.
 */
void nrf_802154_swi_notify_receive_failed(nrf_802154_rx_error_t error);

/**
 * @brief Notify next higher layer that a frame was transmitted from SWI priority level.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of transmitted frame.
 * @param[in]  p_ack    Pointer to buffer containing PSDU of ACK frame. NULL if ACK was not requested.
 * @param[in]  power    RSSI of received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of received frame of 0 if ACK was not requested.
 */
void nrf_802154_swi_notify_transmitted(const uint8_t * p_frame,
                                       uint8_t       * p_data,
                                       int8_t          power,
                                       int8_t          lqi);

/**
 * @brief Notify next higher layer that a frame was not transmitted from SWI priority level.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of the frame that failed transmit
 *                      operation.
 * @param[in]  error    Reason of the transmission failure.
 */
void nrf_802154_swi_notify_transmit_failed(const uint8_t * p_frame, nrf_802154_tx_error_t error);

/**
 * @brief Notify next higher layer that energy detection procedure ended from SWI priority level.
 *
 * @param[in]  result  Detected energy level.
 */
void nrf_802154_swi_notify_energy_detected(uint8_t result);

/**
 * @brief Notify next higher layer that energy detection procedure failed from SWI priority level.
 *
 * @param[in]  error  Reason of the energy detection failure.
 */
void nrf_802154_swi_notify_energy_detection_failed(nrf_802154_ed_error_t error);

/**
 * @brief Notify next higher layer that CCA procedure ended from SWI priority level.
 *
 * @param[in]  channel_free  If detected free channel.
 */
void nrf_802154_swi_notify_cca(bool channel_free);

/**
 * @brief Notify next higher layer that CCA procedure failed from SWI priority level.
 *
 * @param[in]  error  Reason of the CCA failure.
 */
void nrf_802154_swi_notify_cca_failed(nrf_802154_cca_error_t error);

/**
 * @brief Request stop of the HF clock from the SWI priority level.
 *
 * @note This function should be called through notification module to prevent calling it from
 *       the arbiter context.
 */
void nrf_802154_swi_hfclk_stop(void);

/**
 * @brief Terminate stopping of the HF clock.
 *
 * @note This function terminates stopping of the HF clock only if it has not been performed.
 */
void nrf_802154_swi_hfclk_stop_terminate(void);

/**
 * @brief Request entering sleep state from SWI priority.
 *
 * @param[in]   term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[out]  p_result  Result of entering sleep state.
 */
void nrf_802154_swi_sleep(nrf_802154_term_t term_lvl, bool * p_result);

/**
 * @brief Request entering receive state from SWI priority.
 *
 * @param[in]   term_lvl         Termination level of this request. Selects procedures to abort.
 * @param[in]   req_orig         Module that originates this request.
 * @param[in]   notify_function  Function called to notify status of this procedure. May be NULL.
 * @param[in]   notify_abort     If abort notification should be triggered automatically.
 * @param[out]  p_result         Result of entering receive state.
 */
void nrf_802154_swi_receive(nrf_802154_term_t              term_lvl,
                            req_originator_t               req_orig,
                            nrf_802154_notification_func_t notify_function,
                            bool                           notify_abort,
                            bool                         * p_result);

/**
 * @biref Request entering transmit state from SWI priority.
 *
 * @param[in]   term_lvl         Termination level of this request. Selects procedures to abort.
 * @param[in]   req_orig         Module that originates this request.
 * @param[in]   p_data           Pointer to PSDU of the frame to transmit.
 * @param[in]   cca              If the driver should perform CCA procedure before transmission.
 * @param[in]   immediate        If true, the driver schedules transmission immediately or never;
 *                               if false transmission may be postponed until tx preconditions are
 *                               met.
 * @param[in]   notify_function  Function called to notify status of this procedure instead of
 *                               default notification. If NULL default notification is used.
 * @param[out]  p_result         Result of entering transmit state.
 */
void nrf_802154_swi_transmit(nrf_802154_term_t              term_lvl,
                             req_originator_t               req_orig,
                             const uint8_t                * p_data,
                             bool                           cca,
                             bool                           immediate,
                             nrf_802154_notification_func_t notify_function,
                             bool                         * p_result);

/**
 * @brief Request entering energy detection state from SWI priority.
 *
 * @param[in]   term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[in]   time_us   Requested duration of energy detection procedure.
 * @param[out]  p_result  Result of entering energy detection state.
 */
void nrf_802154_swi_energy_detection(nrf_802154_term_t term_lvl,
                                     uint32_t          time_us,
                                     bool            * p_result);

/**
 * @brief Request entering CCA state from SWI priority.
 *
 * @param[in]   term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[out]  p_result  Result of entering CCA state.
 */
void nrf_802154_swi_cca(nrf_802154_term_t term_lvl, bool * p_result);

/**
 * @brief Request entering continuous carrier state from SWI priority.
 *
 * @param[in]   term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[out]  p_result  Result of entering continuous carrier state.
 */
void nrf_802154_swi_continuous_carrier(nrf_802154_term_t term_lvl, bool * p_result);

/**
 * @brief Notify Core module that given buffer is not used anymore and can be freed.
 *
 * @param[in]  p_data  Pointer to the buffer to free.
 */
void nrf_802154_swi_buffer_free(uint8_t * p_data, bool * p_result);

/**
 * @brief Notify Core module that the next higher layer requested channel change.
 */
void nrf_802154_swi_channel_update(bool * p_result);

/**
 * @brief Notify Core module that the next higher layer requested CCA configuration change.
 */
void nrf_802154_swi_cca_cfg_update(bool * p_result);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_802154_SWI_H__

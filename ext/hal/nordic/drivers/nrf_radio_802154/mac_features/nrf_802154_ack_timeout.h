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

#ifndef NRF_802154_ACK_TIMEOUT_H__
#define NRF_802154_ACK_TIMEOUT_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_const.h"
#include "nrf_802154_types.h"

/**
 * @defgroup nrf_802154_csma_ca 802.15.4 driver ACK timeout support
 * @{
 * @ingroup nrf_802154
 * @brief ACK timeout feature.
 */

/**
 * @brief Set timeout time for ACK timeout feature.
 *
 * @param[in]  time  Timeout time in us.
 *                   Default value is defined in nrf_802154_config.h.
 */
void nrf_802154_ack_timeout_time_set(uint32_t time);

/**
 * @brief Abort started ACK timeout procedure.
 *
 * @param[in]  term_lvl  Termination level set by request aborting ongoing operation.
 * @param[in]  req_orig  Module that originates this request.
 *
 * If ACK timeout procedure is not running during call, this function does nothing.
 *
 * @retval  true   ACK timeout procedure han been stopped.
 */
bool nrf_802154_ack_timeout_abort(nrf_802154_term_t term_lvl, req_originator_t req_orig);

/**
 * @brief Handler of transmitted event.
 *
 * @param[in]  p_frame  Pointer to the buffer containing transmitted frame.
 */
void nrf_802154_ack_timeout_transmitted_hook(const uint8_t * p_frame);

/**
 * @brief Handler of TX failed event.
 *
 * @param[in]  p_frame  Pointer to the buffer containing frame that was not transmitted.
 * @param[in]  error    Cause of failed transmission.
 *
 * @retval  true   TX failed event should be propagated to the MAC layer.
 * @retval  false  TX failed event should not be propagated to the MAC layer. It is handled
 *                 internally.
 */
bool nrf_802154_ack_timeout_tx_failed_hook(const uint8_t * p_frame, nrf_802154_tx_error_t error);

/**
 * @brief Handler of TX started event.
 *
 * @param[in]  p_frame  Pointer to the buffer containing frame being transmitted.
 *
 * @retval  true   TX started event should be propagated to the MAC layer.
 * @retval  false  TX started event should not be propagated to the MAC layer. It is handled
 *                 internally.
 */
bool nrf_802154_ack_timeout_tx_started_hook(const uint8_t * p_frame);

/**
 *@}
 **/

#endif // NRF_802154_CSMA_CA_H__

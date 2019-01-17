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

#ifndef NRF_802154_DELAYED_TRX_H__
#define NRF_802154_DELAYED_TRX_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_const.h"
#include "nrf_802154_types.h"

/**
 * @defgroup nrf_802154_delayed_trx Delayed transmission and reception window features.
 * @{
 * @ingroup nrf_802154
 * @brief Delayed transmission or receive window.
 *
 * This module implements delayed transmission and receive window features used in CSL and TSCH
 * modes.
 */

/**
 * @brief Request transmission of a frame at given time.
 *
 * If requested transmission is successful and the frame is transmitted the
 * @ref nrf_802154_tx_started is called. If the requested frame cannot be transmitted at given time
 * the @ref nrf_802154_transmit_failed function is called.
 *
 * @note Delayed transmission does not timeout waiting for ACK automatically. Waiting for ACK shall
 *       be timed out by the next higher layer or the ACK timeout module. The ACK timeout timer
 *       shall start when the @ref nrf_802154_tx_started function is called.
 *
 * @param[in]  p_data   Pointer to array containing data to transmit (PHR + PSDU).
 * @param[in]  cca      If the driver should perform CCA procedure before transmission.
 * @param[in]  t0       Base of delay time [us].
 * @param[in]  dt       Delta of delay time from @p t0 [us].
 * @param[in]  channel  Number of channel on which the frame should be transmitted.
 */
bool nrf_802154_delayed_trx_transmit(const uint8_t * p_data,
                                     bool            cca,
                                     uint32_t        t0,
                                     uint32_t        dt,
                                     uint8_t         channel);

/**
 *@}
 **/

/**
 * @brief Request reception of a frame at given time.
 *
 * If the requested is accepted and a frame is received during defined time slot
 * @ref nrf_802154_received is called. If the request is rejected due to denied timeslot request
 * or reception timeout expired the @ref nrf_802154_receive_failed function is called.
 *
 * @param[in]  t0       Base of delay time [us].
 * @param[in]  dt       Delta of delay time from @p t0 [us].
 * @param[in]  timeout  Reception timeout (counted from @p t0 + @p dt) [us].
 * @param[in]  channel  Number of channel on which the frame should be received.
 */
bool nrf_802154_delayed_trx_receive(uint32_t t0,
                                    uint32_t dt,
                                    uint32_t timeout,
                                    uint8_t  channel);

/**
 * @brief Abort started delayed transmit/receive procedure.
 *
 * @param[in]  term_lvl  Termination level set by request aborting ongoing operation.
 * @param[in]  req_orig  Module that originates this request.
 *
 * If delayed transmit/receive procedure are not running during call, this function does nothing.
 *
 * @retval  true   transmit/receive procedures have been stopped.
 */
bool nrf_802154_delayed_trx_abort(nrf_802154_term_t term_lvl, req_originator_t req_orig);

/**
 * @brief Extends timeout timer when reception start is detected and there is not enough time
 *        left for delayed RX operation.
 *
 * If delayed transmit/receive procedure are not running during call, this function does nothing.
 */
void nrf_802154_delayed_trx_rx_started_hook(void);

/**
 *@}
 **/

#endif // NRF_802154_DELAYED_TRX_H__

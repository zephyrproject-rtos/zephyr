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

#ifndef NRF_802154_CORE_HOOKS_H__
#define NRF_802154_CORE_HOOKS_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_const.h"
#include "nrf_802154_types.h"

/**
 * @defgroup nrf_802154_hooks Hooks for the 802.15.4 driver core
 * @{
 * @ingroup nrf_802154
 * @brief Hooks for the 802.15.4 driver core module.
 *
 * Hooks are used by optional driver features to modify way in which notifications are propagated
 * through the driver.
 */

/**
 * @brief Process hooks for the terminate request.
 *
 * @param[in]     term_lvl  Termination level of request that terminates current operation.
 * @param[in]     req_orig  Module that originates this request.
 *
 * @retval true   All procedures are aborted.
 * @retval false  There is ongoing procedure that cannot be aborted due to too low @p priority.
 */
bool nrf_802154_core_hooks_terminate(nrf_802154_term_t term_lvl, req_originator_t req_orig);

/**
 * @brief Process hooks for the transmitted event.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of the frame that was transmitted.
 */
void nrf_802154_core_hooks_transmitted(const uint8_t * p_frame);

/**
 * @brief Process hooks for the TX failed event.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of the frame that was not transmitted.
 * @param[in]  error    Cause of failed transmission.
 *
 * @retval  true   TX failed event should be propagated to the MAC layer.
 * @retval  false  TX failed event should not be propagated to the MAC layer. It is handled
 *                 internally.
 */
bool nrf_802154_core_hooks_tx_failed(const uint8_t * p_frame, nrf_802154_tx_error_t error);

/**
 * @brief Process hooks for the TX started event.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of the frame that is being transmitted.
 *
 * @retval  true   TX started event should be propagated to the MAC layer.
 * @retval  false  TX started event should not be propagated to the MAC layer. It is handled
 *                 internally.
 */
bool nrf_802154_core_hooks_tx_started(const uint8_t * p_frame);

/**
 * @brief Process hooks for the RX started event.
 */
void nrf_802154_core_hooks_rx_started(void);

/**
 *@}
 **/

#endif // NRF_802154_CORE_HOOKS_H__

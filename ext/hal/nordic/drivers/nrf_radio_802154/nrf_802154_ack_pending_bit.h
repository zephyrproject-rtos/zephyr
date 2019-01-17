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
 * @brief This file implements procedures to set pending bit in nRF 802.15.4 radio driver.
 *
 */

#ifndef NRF_802154_ACK_PENDING_BIT_H_
#define NRF_802154_ACK_PENDING_BIT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize this module.
 */
void nrf_802154_ack_pending_bit_init(void);

/**
 * @brief Enable or disable procedure of setting pending bit in ACK frame.
 *
 * @param[in]  enabled  True if procedure should be enabled, false otherwise.
 */
void nrf_802154_ack_pending_bit_set(bool enabled);

/**
 * @brief Add address to ACK pending bit list.
 *
 * ACK frames sent in response to frames with source address matching any address from ACK pending
 * bit list will have pending bit set. If source address does not match any of the addresses in the
 * list the ACK frame will have pending bit cleared.
 *
 * @param[in]  p_addr    Pointer to address that should be added to the list.
 * @param[in]  extended  Indication if @p p_addr is extended or short address.
 *
 * @retval true   Address successfully added to the list.
 * @retval false  Address was not added to the list (list is full).
 */
bool nrf_802154_ack_pending_bit_for_addr_set(const uint8_t * p_addr, bool extended);

/**
 * @brief Remove address from ACK pending bit list.
 *
 * ACK frames sent in response to frames with source address matching any address from ACK pending
 * bit list will have pending bit set. If source address does not match any of the addresses in the
 * list the ACK frame will have pending bit cleared.
 *
 * @param[in]  p_addr    Pointer to address that should be removed from the list.
 * @param[in]  extended  Indication if @p p_addr is extended or short address.
 *
 * @retval true   Address successfully removed from the list.
 * @retval false  Address was not removed from the list (address is missing in the list).
 */
bool nrf_802154_ack_pending_bit_for_addr_clear(const uint8_t * p_addr, bool extended);

/**
 * @brief Remove all addresses of given length from ACK pending bit list.
 *
 * @param[in]  extended  Indication if all extended or all short addresses should be removed from
 *                       the list.
 */
void nrf_802154_ack_pending_bit_for_addr_reset(bool extended);

/**
 * @brief Check if pending bit should be set in ACK sent in response to given frame.
 *
 * @param[in]  p_psdu  PSDU of frame to which ACK frame is being prepared.
 *
 * @retval true   Pending bit should be set.
 * @retval false  Pending bit should be cleared.
 */
bool nrf_802154_ack_pending_bit_should_be_set(const uint8_t * p_psdu);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_ACK_PENDING_BIT_H_ */

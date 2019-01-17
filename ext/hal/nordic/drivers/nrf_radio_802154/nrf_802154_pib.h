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
 * @brief This file implements storage of PIB attributes in nRF 802.15.4 radio driver.
 *
 */

#ifndef NRF_802154_PIB_H_
#define NRF_802154_PIB_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize this module.
 */
void nrf_802154_pib_init(void);

/**
 * @brief Check if promiscuous mode is enabled.
 *
 * @retval  true   If promiscuous mode is enabled.
 * @retval  false  If promiscuous mode is disabled.
 */
bool nrf_802154_pib_promiscuous_get(void);

/**
 * @brief Enable or disable promiscuous mode.
 *
 * @param[in]  enabled  If promiscuous mode should be enabled.
 */
void nrf_802154_pib_promiscuous_set(bool enabled);

/**
 * @brief Check if auto ACK procedure is enabled.
 *
 * @retval  true   If auto ACK procedure is enabled.
 * @retval  false  If auto ACK procedure is disabled.
 */
bool nrf_802154_pib_auto_ack_get(void);

/**
 * @brief Enable or disable auto ACK procedure.
 *
 * @param[in]  enabled  If auto ACK procedure should be enabled.
 */
void nrf_802154_pib_auto_ack_set(bool enabled);

/**
 * @brief Check if radio is configured as the PAN coordinator.
 *
 * @retval  true   If radio is configured as the PAN coordinator.
 * @retval  false  If radio is not configured as the PAN coordinator.
 */
bool nrf_802154_pib_pan_coord_get(void);

/**
 * @brief Notify driver that radio is configured as the PAN coordinator.
 *
 * @param[in]  enabled  If radio is configured as the PAN coordinator.
 */
void nrf_802154_pib_pan_coord_set(bool enabled);

/**
 * @brief Get currently used channel.
 *
 * @return  Channel number used by the driver.
 */
uint8_t nrf_802154_pib_channel_get(void);

/**
 * @brief Set channel that will be used by the driver.
 *
 * @param[in]  channel  Number of channel used by the driver.
 */
void nrf_802154_pib_channel_set(uint8_t channel);

/**
 * @brief Get transmit power.
 *
 * @returns  Transmit power [dBm].
 */
int8_t nrf_802154_pib_tx_power_get(void);

/**
 * @brief Set transmit power used for ACK frames.
 *
 * @param[in]  dbm  Transmit power [dBm].
 */
void nrf_802154_pib_tx_power_set(int8_t dbm);

/**
 * @brief Get PAN Id used by this device.
 *
 * @returns Pointer to buffer containing PAN Id value (2 bytes, little-endian).
 */
const uint8_t * nrf_802154_pib_pan_id_get(void);

/**
 * @brief Set PAN Id used by this device.
 *
 * @param[in]  p_pan_id  Pointer to PAN Id (2 bytes, little-endian).
 *
 * This function makes copy of the PAN Id.
 */
void nrf_802154_pib_pan_id_set(const uint8_t * p_pan_id);

/**
 * @brief Get Extended Address of this device
 *
 * @returns Pointer to buffer containing extended address (8 bytes, little-endian).
 */
const uint8_t * nrf_802154_pib_extended_address_get(void);

/**
 * @brief Set Extended Address of this device.
 *
 * @param[in]  p_extended_address  Pointer to extended address (8 bytes, little-endian).
 *
 * This function makes copy of the address.
 */
void nrf_802154_pib_extended_address_set(const uint8_t * p_extended_address);

/**
 * @brief Get Short Address of this device
 *
 * @returns Pointer to buffer containing short address (2 bytes, little-endian).
 */
const uint8_t * nrf_802154_pib_short_address_get(void);

/**
 * @brief Set Short Address of this device.
 *
 * @param[in]  p_short_address  Pointer to short address (2 bytes, little-endian).
 *
 * This function makes copy of the address.
 */
void nrf_802154_pib_short_address_set(const uint8_t * p_short_address);

/**
 * @brief Set radio CCA mode and threshold.
 *
 * @param[in] p_cca_cfg A pointer to the CCA configuration structure. Only fields relevant to selected mode are updated.
 */
void nrf_802154_pib_cca_cfg_set(const nrf_802154_cca_cfg_t * p_cca_cfg);

/**
 * @brief Get current radio CCA configuration.
 *
 * @param[out] p_cca_cfg A pointer to the structure for current CCA configuration.
 */
void nrf_802154_pib_cca_cfg_get(nrf_802154_cca_cfg_t * p_cca_cfg);

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_PIB_H_ */

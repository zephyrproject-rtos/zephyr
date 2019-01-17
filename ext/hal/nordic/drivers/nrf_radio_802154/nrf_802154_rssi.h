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

#ifndef NRF_802154_RSSI_H__
#define NRF_802154_RSSI_H__

#include <stdint.h>

/**
 * @defgroup nrf_802154_rssi RSSI calculations used internally in the 802.15.4 driver.
 * @{
 * @ingroup nrf_802154
 * @brief RSSI calculations used internally in the 802.15.4 driver.
 */

/**
 * @brief Get RSSISAMPLE temperature correction value for temperature provided by platform.
 *
 * @returns RSSISAMPLE temperature correction value (Errata 153).
 */
int8_t nrf_802154_rssi_sample_temp_corr_value_get(void);

/**
 * @brief Adjust the given RSSISAMPLE value using a temperature correction factor.
 *
 * @param[in]  rssi_sample  Value read from RSSISAMPLE register.
 *
 * @returns RSSISAMPLE corrected by a temperature factor (Errata 153).
 */
uint8_t nrf_802154_rssi_sample_corrected_get(uint8_t rssi_sample);

/**
 * @brief Adjust the reported LQI value using a temperature correction factor.
 *
 * @param[in]  lqi   Value read from LQI byte.
 *
 * @returns LQI byte value corrected by a temperature factor (Errata 153).
 */
uint8_t nrf_802154_rssi_lqi_corrected_get(uint8_t lqi);

/**
 * @brief Adjust the EDSAMPLE value using a temperature correction factor.
 *
 * @param[in]  ed    Value read from EDSAMPLE register.
 *
 * @returns EDSAMPLE value corrected by a temperature factor (Errata 153).
 */
uint8_t nrf_802154_rssi_ed_corrected_get(uint8_t ed);

/**
 * @brief Adjust the CCA ED threshold value using a temperature correction factor.
 *
 * @param[in]  cca_ed  Value representing CCA ED threshold that should be corrected.
 *
 * @returns CCA ED threshold value corrected by a temperature factor (Errata 153).
 */
uint8_t nrf_802154_rssi_cca_ed_threshold_corrected_get(uint8_t cca_ed);

/**
 *@}
 **/

#endif // NRF_802154_RSSI_H__

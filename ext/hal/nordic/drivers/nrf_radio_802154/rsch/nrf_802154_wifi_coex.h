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

/**
 * @brief This module defines the Wifi coexistence module.
 *
 */

#ifndef NRF_802154_WIFI_COEX_H_
#define NRF_802154_WIFI_COEX_H_

#include "nrf_802154_rsch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_wifi_coex Wifi Coexistence
 * @{
 * @ingroup nrf_802154
 * @brief Wifi Coexistence module.
 *
 * Wifi Coexistence module is a client of the PTA (defined in the 802.15.2). It manages GPIO
 * to assert pins and respond to pin state changes.
 */

/**
 * @brief Initialize the Wifi Coexistence module.
 *
 * @note This function shall be called once, before any other function from this module.
 *
 */
void nrf_802154_wifi_coex_init(void);

/**
 * @brief Uninitialize the Wifi Coexistence module.
 *
 */
void nrf_802154_wifi_coex_uninit(void);

/**
 * @brief Request given priority from the Wifi Coexistence module.
 *
 * @note The approval of requested priority is notified asynchronously by the
 *       @ref nrf_802154_wifi_coex_prio_changed call.
 *
 * @param[in]  priority  Requested priority level.
 *
 */
void nrf_802154_wifi_coex_prio_req(rsch_prio_t priority);

/**
 * @brief Get priority denial event address.
 *
 * Get an address of a hardware event that notifies about denial of the previously approved
 * priority.
 *
 * @return Address of a priority denial event.
 */
void * nrf_802154_wifi_coex_deny_event_addr_get(void);

/**
 * @biref Approved priority change notification.
 *
 * The Wifi Coexistence module calls this function to notify the RSCH of currently approved
 * priority level.
 *
 * @param[in]  priority  Approved priority level.
 */
extern void nrf_802154_wifi_coex_prio_changed(rsch_prio_t priority);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_WIFI_COEX_H_ */

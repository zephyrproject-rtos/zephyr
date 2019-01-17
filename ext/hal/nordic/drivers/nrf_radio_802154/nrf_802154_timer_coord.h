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
 * @brief This module defines the Timer Coordinator interface.
 *
 */

#ifndef NRF_802154_TIMER_COORD_H_
#define NRF_802154_TIMER_COORD_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_timer_coord Timer Coordinator
 * @{
 * @ingroup nrf_802154
 * @brief Timer Coordinator interface.
 *
 * Timer Coordinator is responsible to synchronize and coordinate operations of the Low Power timer
 * that counts absolute time and the High Precision timer that counts time relative to a timeslot.
 */

/**
 * @brief Initialize the Timer Coordinator module.
 *
 */
void nrf_802154_timer_coord_init(void);

/**
 * @brief Uninitialize the Timer Coordinator module.
 *
 */
void nrf_802154_timer_coord_uninit(void);

/**
 * @brief Start the Timer Coordinator.
 *
 * This function starts the HP timer and synchronizes it with the LP timer.
 *
 * Started Timer Coordinator resynchronizes automatically in constant interval.
 */
void nrf_802154_timer_coord_start(void);

/**
 * @brief Stop the Timer Coordinator.
 *
 * This function stops the HP timer.
 */
void nrf_802154_timer_coord_stop(void);

/**
 * @brief Prepare getting precise timestamp of given event.
 *
 * @param[in]  event_addr  Address of the peripheral register corresponding to the event that
 *                         should be timestamped.
 */
void nrf_802154_timer_coord_timestamp_prepare(uint32_t event_addr);

/**
 * @brief Get timestamp of the recently prepared event.
 *
 * If recently prepared event occurred a few times since preparation, this function returns
 * timestamp of the first occurrence.
 * If the requested event did not occur since preparation or HP timer is not synchronized, this
 * function returns false.
 *
 * @param[out]  p_timestamp  Precise absolute timestamp of recently prepared event [us].
 *
 * @retval true   Timestamp is available.
 * @retval false  Timestamp is unavailable.
 */
bool nrf_802154_timer_coord_timestamp_get(uint32_t * p_timestamp);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_TIMER_COORD_H_ */

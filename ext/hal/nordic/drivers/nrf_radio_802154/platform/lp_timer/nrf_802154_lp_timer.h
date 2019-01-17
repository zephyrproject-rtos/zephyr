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
 * @brief This module defines Low Power Timer Abstraction Layer for the 802.15.4 driver.
 *
 */

#ifndef NRF_802154_LP_TIMER_API_H_
#define NRF_802154_LP_TIMER_API_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_timer Low Power Timer Abstraction Layer for the 802.15.4 driver
 * @{
 * @ingroup nrf_802154_timer
 * @brief Low Power Timer Abstraction Layer interface for the 802.15.4 driver.
 *
 * Low Power Timer Abstraction Layer is an abstraction layer of timer that is meant to be used by
 * the nRF 802.15.4 driver. This timer should provide low latency (max 100 us) in order to allow
 * implementation in the driver code features like:
 * * Timing out waiting for ACK frame
 * * SIFS and LIFS
 * * CSMA/CA
 * * CSL
 * * Auto polling by rx-off-when-idle devices
 *
 * @note Most of Low Power Timer Abstraction Layer API should not be called directly by 802.15.4 driver
 *       modules. This API is used by the Timer Scheduler module included in the driver and other
 *       modules should use Timer Scheduler API. Exception from above rule are initialization and
 *       deinitialization functions @sa nrf_802154_lp_timer_init()
 *       @sa nrf_802154_lp_timer_deinit() and critical section management
 *       @sa nrf_802154_lp_timer_critical_section_enter()
 *       @sa nrf_802154_lp_timer_critical_section_exit() as these functions are called from
 *       nrf_802154_critical_section module and from global initialization functions
 *       @sa nrf_802154_init() @sa nrf_802154_deinit().
 */

/**
 * @brief Initialize the Timer.
 */
void nrf_802154_lp_timer_init(void);

/**
 * @brief Uninitialize the Timer.
 */
void nrf_802154_lp_timer_deinit(void);

/**
 * @brief Enter critical section of the timer.
 *
 * In critical section timer cannot execute @sa nrf_802154_lp_timer_fired() function.
 *
 * @note Critical section cannot be nested.
 */
void nrf_802154_lp_timer_critical_section_enter(void);

/**
 * @brief Exit critical section of the timer.
 *
 * In critical section timer cannot execute @sa nrf_802154_lp_timer_fired() function.
 *
 * @note Critical section cannot be nested.
 */
void nrf_802154_lp_timer_critical_section_exit(void);

/**
 * @brief Get current time.
 *
 * Prior to getting current time, Timer must be initialized @sa nrf_802154_lp_timer_init().
 * There are no other requirements that must be fulfilled before using this function.
 *
 * @return Current time in microseconds [us].
 */
uint32_t nrf_802154_lp_timer_time_get(void);

/**
 * @brief Get granularity of currently used timer.
 *
 * This function may be used to round up/down time calculations.
 *
 * @return Timer granularity in microseconds [us].
 */
uint32_t nrf_802154_lp_timer_granularity_get(void);

/**
 * @brief Start one-shot timer that expires at specified time.
 *
 * Start one-shot timer that will expire @p dt microseconds after @p t0 time.
 * If timer is running when this function is called, previously running timer will be stopped
 * automatically.
 *
 * On timer expiration @sa nrf_802154_lp_timer_fired function will be called.
 * Timer automatically stops after expiration.
 *
 * @param[in]  t0  Number of microseconds representing timer start time.
 * @param[in]  dt  Time of timer expiration as time elapsed from @p t0 [us].
 */
void nrf_802154_lp_timer_start(uint32_t t0, uint32_t dt);

/**
 * @brief Stop currently running timer.
 */
void nrf_802154_lp_timer_stop(void);

/**
 * @brief Check if timer is currently running.
 *
 * @retval  true   Timer is running.
 * @retval  false  Timer is not running.
 */
bool nrf_802154_lp_timer_is_running(void);

/**
 * @brief Start one-shot synchronization timer that expires at nearest possible timepoint.
 *
 * On timer expiration @ref nrf_802154_lp_timer_synchronized function will be called and
 * event returned by @ref nrf_802154_lp_timer_sync_event_get will be triggered.
 *
 * @note @ref nrf_802154_lp_timer_synchronized may be called multiple times.
 */
void nrf_802154_lp_timer_sync_start_now(void);

/**
 * @brief Start one-shot synchronization timer that expires at specified time.
 *
 * Start one-shot synchronization timer that will expire @p dt microseconds after @p t0 time.
 *
 * On timer expiration @ref nrf_802154_lp_timer_synchronized function will be called and
 * event returned by @ref nrf_802154_lp_timer_sync_event_get will be triggered.
 *
 * @param[in]  t0  Number of microseconds representing timer start time.
 * @param[in]  dt  Time of timer expiration as time elapsed from @p t0 [us].
 */
void nrf_802154_lp_timer_sync_start_at(uint32_t t0, uint32_t dt);

/**
 * @brief Stop currently running synchronization timer.
 */
void nrf_802154_lp_timer_sync_stop(void);

/**
 * @brief Get event used to synchronize this timer with HP Timer
 *
 * @return  Address of the peripheral register corresponding to the event that
 *          should be used for timers synchronization.
 */
uint32_t nrf_802154_lp_timer_sync_event_get(void);

/**
 * @brief Get timestamp of the synchronization event.
 *
 * @return  Timestamp of the synchronization event.
 */
uint32_t nrf_802154_lp_timer_sync_time_get(void);

/**
 * @brief Callback executed when timer expires.
 */
extern void nrf_802154_lp_timer_fired(void);

/**
 * @brief Callback executed when synchronization timer expires.
 */
extern void nrf_802154_lp_timer_synchronized(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_LP_TIMER_API_H_ */

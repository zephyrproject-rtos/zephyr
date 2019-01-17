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
 * @brief This module defines Clock Abstraction Layer for the 802.15.4 driver.
 *
 * Clock Abstraction Layer can be used by other modules to start and stop nRF52840 clocks.
 *
 * It is used by Radio Arbiter clients (RAAL) to start HF clock when entering continuous mode
 * and stop HF clock after continuous mode exit.
 *
 * It is used by standalone Low Power Timer Abstraction Layer implementation
 * to start LF clock during initialization.
 *
 */

#ifndef NRF_802154_CLOCK_H_
#define NRF_802154_CLOCK_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_clock Clock Abstraction Layer for the 802.15.4 driver
 * @{
 * @ingroup nrf_802154_clock
 * @brief Clock Abstraction Layer interface for the 802.15.4 driver.
 *
 */

/**
 * @brief Initialize the clock driver.
 */
void nrf_802154_clock_init(void);

/**
 * @brief Deinitialize the clock driver.
 */
void nrf_802154_clock_deinit(void);

/**
 * @brief Start High Frequency Clock.
 *
 * This function is asynchronous. It should request ramping up of HF clock and exit. When HF clock
 * is ready @sa nrf_802154_hfclk_ready() should be called.
 */
void nrf_802154_clock_hfclk_start(void);

/**
 * @brief Stop High Frequency Clock.
 */
void nrf_802154_clock_hfclk_stop(void);

/**
 * @brief Check if High Frequency Clock is running.
 *
 * @retval true  If High Frequency Clock is running.
 * @retval false If High Frequency Clock is not running.
 */
bool nrf_802154_clock_hfclk_is_running(void);

/**
 * @brief Start Low Frequency Clock.
 *
 * This function is asynchronous. It should request ramping up of LF clock and exit. When LF clock
 * is ready @sa nrf_802154_lfclk_ready() should be called.
 */
void nrf_802154_clock_lfclk_start(void);

/**
 * @brief Stop Low Frequency Clock.
 */
void nrf_802154_clock_lfclk_stop(void);

/**
 * @brief Check if Low Frequency Clock is running.
 *
 * @retval true  If Low Frequency Clock is running.
 * @retval false If Low Frequency Clock is not running.
 */
bool nrf_802154_clock_lfclk_is_running(void);

/**
 * @brief Callback executed when High Frequency Clock is ready.
 */
extern void nrf_802154_clock_hfclk_ready(void);

/**
 * @brief Callback executed when Low Frequency Clock is ready.
 */
extern void nrf_802154_clock_lfclk_ready(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_CLOCK_H_ */

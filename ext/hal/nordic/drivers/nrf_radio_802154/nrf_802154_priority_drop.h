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

#ifndef NRF_802154_PRIORITY_DROP_H__
#define NRF_802154_PRIORITY_DROP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_priority_drop 802.15.4 driver procedures with lower priority.
 * @{
 * @ingroup nrf_802154
 * @brief Internal procedures of 802.15.4 driver that should be called with lower priority than
 *        the caller's priority.
 */

/**
 * @brief Initialize notification module.
 */
void nrf_802154_priority_drop_init(void);

/**
 * @brief Request stop of the high frequency clock.
 *
 * @note This function should be called through this module to prevent calling it from the arbiter
 *       context.
 */
void nrf_802154_priority_drop_hfclk_stop(void);

/**
 * @brief Terminate requesting of high frequency clock stop.
 *
 * Function used to to terminate HFClk stop procedure requested by previous call to
 * @rev nrf_802154_priority_drop_hfclk_stop. The HFClk stop procedure is terminated only if it has
 * not been started.
 */
void nrf_802154_priority_drop_hfclk_stop_terminate(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_802154_PRIORITY_DROP_H__

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
 * @brief This module defines Radio Arbiter Abstraction Layer interface.
 *
 */

#ifndef NRF_RAAL_API_H_
#define NRF_RAAL_API_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_raal Radio Arbiter Abstraction Layer
 * @{
 * @ingroup nrf_802154
 * @brief Radio Arbiter Abstraction Layer interface.
 */

/**
 * @brief Initialize RAAL.
 *
 * @note This function shall be called once, before any other function from this module.
 *
 * Initialize Radio Arbiter Abstraction Layer client.
 *
 * @note Arbiter starts in inactive mode after initialization. In order to start radio activity
 *       @p nrf_raal_continuous_mode_enter method should be called.
 *
 */
void nrf_raal_init(void);

/**
 * @brief Uninitialize RAAL.
 *
 * Uninitialize Radio Arbiter Abstraction Layer client.
 *
 */
void nrf_raal_uninit(void);

/**
 * @brief Enter arbiter into continuous radio mode.
 *
 * In this mode radio arbiter should try to create long continuous timeslots that will give the
 * radio driver as much radio time as possible while disturbing the other activities as little
 * as possible.
 * The arbiter client shall make sure that high frequency clock is enabled in each timeslot.
 *
 * @note The start of a timeslot will be indicated by @p nrf_raal_timeslot_started call.
 *
 */
void nrf_raal_continuous_mode_enter(void);

/**
 * @brief Exit arbiter from continuous mode.
 *
 * In this mode radio arbiter will not extend or allocate any more timeslots for radio driver.
 *
 */
void nrf_raal_continuous_mode_exit(void);

/**
 * @brief Confirm to RAAL that current part of the continuous timeslot is ended.
 *
 * The core cannot use the RADIO peripheral after this call until the timeslot is started again.
 */
void nrf_raal_continuous_ended(void);

/**
 * @brief Request timeslot for radio communication.
 *
 * This method should be called only after @p nrf_raal_timeslot_started indicated the start
 * of a timeslot.
 *
 * @param[in] length_us  Requested radio timeslot length in microsecond.
 *
 * @retval TRUE  The radio driver now has exclusive access to the RADIO peripheral for the
 *               full length of the timeslot.
 * @retval FALSE Slot cannot be assigned due to other activities.
 *
 */
bool nrf_raal_timeslot_request(uint32_t length_us);

/**
 * @brief Get left time of currently granted timeslot [us].
 *
 * @returns  Number of microseconds left in currently granted timeslot.
 */
uint32_t nrf_raal_timeslot_us_left_get(void);

/**
 * @brief Enter critical section of the module.
 *
 * When this method is called, the execution of the @sa nrf_raal_timeslot_started and
 * @sa nrf_raal_timeslot_ended function is blocked.
 *
 * @note This function may be called when RAAL is already in critical section. Ongoing call may
 *       be interrupted by another call from IRQ with higher priority.
 *
 */
void nrf_raal_critical_section_enter(void);

/**
 * @brief Exit critical section of the module.
 *
 * When this method is called driver has to expect the execution of the
 * @sa nrf_raal_timeslot_started or @sa nrf_raal_timeslot_ended
 * function.
 *
 * @note This function may be called when RAAL has already exited critical section. Ongoing call
 *       may NOT be interrupted by another call from IRQ with higher priority.
 */
void nrf_raal_critical_section_exit(void);

/**
 * @brief RAAL client calls this method to notify radio driver about the start of a timeslot.
 *
 * The radio driver now has exclusive access to the peripherals until @p nrf_raal_timeslot_ended
 * is called.
 *
 * @note The high frequency clock must be enabled when this function is called.
 * @note The end of the timeslot will be indicated by @p nrf_raal_timeslot_ended function.
 *
 */
extern void nrf_raal_timeslot_started(void);

/**
 * @brief RAAL client calls this method to notify radio driver about the end of a timeslot.
 *
 * Depending on the RAAL client configuration, radio driver has NRF_RAAL_MAX_CLEAN_UP_TIME_US
 * microseconds to do any clean-up actions on RADIO peripheral and stop using it completely.
 * Thus arbiter has to call this function NRF_RAAL_MAX_CLEAN_UP_TIME_US microseconds before
 * timeslot is finished.
 *
 * If RAAL is in the continuous mode, the next timeslot will be indicated again by
 * @p nrf_raal_timeslot_started function.
 *
 * This method shall not be called if @p nrf_raal_continuous_mode_exit has been called. Radio
 * driver shall assume that timeslot has been finished after @p nrf_raal_continuous_mode_exit
 * call.
 *
 * @note Because radio driver needs to stop any operation on RADIO peripheral within
 *       NRF_RAAL_MAX_CLEAN_UP_TIME_US microseconds, this method should be called with high
 *       interrupt priority level to avoid unwanted delays.
 *
 */
extern void nrf_raal_timeslot_ended(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_RAAL_API_H_ */

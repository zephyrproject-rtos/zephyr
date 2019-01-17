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
 * @brief This module defines Radio Scheduler interface.
 *
 */

#ifndef NRF_802154_RSCH_H_
#define NRF_802154_RSCH_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_rsch Radio Scheduler
 * @{
 * @ingroup nrf_802154
 * @brief Radio Scheduler interface.
 *
 * Radio Scheduler is responsible to schedule in time radio activities and preconditions. It is
 * expected that the Radio Scheduler module manages timings to meet requirements requested from the
 * core module.
 *
 * Examples of radio activity preconditions are: High-Frequency Clock running, radio arbiter (RAAL)
 * granted access to the RADIO peripheral.
 */

/**
 * @brief List of preconditions that have to be met before any radio activity.
 */
typedef enum
{
    RSCH_PREC_HFCLK,
    RSCH_PREC_RAAL,
    RSCH_PREC_CNT,
} rsch_prec_t;

/**
 * @brief Priorities of the 802.15.4 radio operations.
 */
typedef enum
{
    RSCH_PRIO_IDLE,                                    ///< Priority used in the sleep state. With this priority RSCH releases all preconditions.
    RSCH_PRIO_IDLE_LISTENING,                          ///< Priority used during the idle listening procedure.
    RSCH_PRIO_RX,                                      ///< Priority used when a frame is being received.
    RSCH_PRIO_DETECT,                                  ///< Priority used to detect channel conditions (CCA, ED).
    RSCH_PRIO_TX,                                      ///< Priority used to transmit a frame.

    RSCH_PRIO_MIN_APPROVED = RSCH_PRIO_IDLE_LISTENING, ///< Minimal priority indicating that given precondition is approved.
    RSCH_PRIO_MAX          = RSCH_PRIO_TX,             ///< Maximal priority available in the RSCH module.
} rsch_prio_t;

/**
 * @brief Enumeration of delayed timeslots ids.
 */
typedef enum
{
    RSCH_DLY_TX,     ///< Timeslot for delayed tx operation.
    RSCH_DLY_RX,     ///< Timeslot for delayed rx operation.

    RSCH_DLY_TS_NUM, ///< Number of delayed timeslots.
} rsch_dly_ts_id_t;

/**
 * @brief Initialize Radio Scheduler.
 *
 * @note This function shall be called once, before any other function from this module.
 *
 * Initialize Radio Scheduler.
 *
 * @note Radio Scheduler starts in inactive mode after initialization. In order to start radio activity
 *       @ref nrf_802154_rsch_continuous_mode_enter should be called.
 *
 */
void nrf_802154_rsch_init(void);

/**
 * @brief Uninitialize Radio Scheduler.
 *
 */
void nrf_802154_rsch_uninit(void);

/**
 * @brief Set priority for the continuous radio mode.
 *
 * In the continuous mode the radio scheduler should try to satisfy all preconditions as long as
 * possible in order to give to the radio driver core as much radio time as possible while
 * disturbing the other activities as little as possible.
 *
 * @note The start of a timeslot will be indicated by @ref nrf_802154_rsch_prec_approved call.
 * @note To disable the continuous radio mode, the @ref RSCH_PRIO_IDLE should be used.
 *
 * @param[in]  prio  Priority level used in the continuous radio mode.
 *
 */
void nrf_802154_rsch_continuous_mode_priority_set(rsch_prio_t prio);

/**
 * @brief Confirm that current part of continuous timeslot is ended by the core.
 *
 * This confirmation is used by the core to synchronize ending of continuous timeslot parts with
 * the RSCH module.
 *
 */
void nrf_802154_rsch_continuous_ended(void);

/**
 * @brief Request timeslot for radio communication immediately.
 *
 * This function should be called only after @ref nrf_802154_rsch_prec_approved indicated the
 * start of a timeslot.
 *
 * @param[in] length_us  Requested radio timeslot length in microsecond.
 *
 * @retval true   The radio driver now has exclusive access to the RADIO peripheral for the
 *                full length of the timeslot.
 * @retval false  Slot cannot be assigned due to other activities.
 *
 */
bool nrf_802154_rsch_timeslot_request(uint32_t length_us);

/**
 * @brief Request timeslot in the future.
 *
 * Request timeslot that should be granted in the future. Function parameters provides data when
 * the timeslot should start and how long should it last. When requested timeslot starts the
 * @ref nrf_802154_rsch_delayed_timeslot_started is called. If requested timeslot cannot be granted
 * with requested parameters, the @ref nrf_802154_rsch_delayed_timeslot_failed is called.
 *
 * @note Time parameters use the same units that are used in the Timer Scheduler module.
 *
 * @param[in]  t0      Base time of the timestamp of the timeslot start [us].
 * @param[in]  dt      Time delta between @p t0 and the timestamp of the timeslot start [us].
 * @param[in]  length  Requested radio timeslot length [us].
 * @param[in]  prio    Priority level required for the delayed timeslot.
 * @param[in]  dly_ts  Type of the requested timeslot.
 *
 * @retval true   Requested timeslot has been scheduled.
 * @retval false  Requested timeslot cannot be scheduled and will not be granted.
 */
bool nrf_802154_rsch_delayed_timeslot_request(uint32_t         t0,
                                              uint32_t         dt,
                                              uint32_t         length,
                                              rsch_prio_t      prio,
                                              rsch_dly_ts_id_t dly_ts);

/**
 * @brief Check if the RSCH precondition is satisfied.
 *
 * @param[in]  prec    RSCH precondition to be checked.
 * @param[in]  prio    Minimal required priority level of given precondition.
 *
 * @retval true   Precondition @p prec is currently granted.
 * @retval false  Precondition @p prec is not currently granted.
 */
bool nrf_802154_rsch_prec_is_approved(rsch_prec_t prec, rsch_prio_t prio);

/**
 * @brief Get left time of currently granted timeslot [us].
 *
 * @returns  Number of microseconds left in currently granted timeslot.
 */
uint32_t nrf_802154_rsch_timeslot_us_left_get(void);

/**
 * @brief This function is called to notify the core about changes of approved priority level.
 *
 * If the @p prio is greater than @ref RSCH_PRIO_IDLE, the radio driver has exclusive access to the
 * peripherals until this function is called with the @p prio equal to @ref RSCH_PRIO_IDLE.
 *
 * @note The end of the timeslot is indicated by @p prio equal to the @ref RSCH_PRIO_IDLE.
 *
 * @param[in]  prio  Currently approved priority level.
 */
extern void nrf_802154_rsch_continuous_prio_changed(rsch_prio_t prio);

/**
 * @brief Notification that previously requested delayed timeslot has started just now.
 *
 * @param[in]  dly_ts_id  Type of the started timeslot.
 */
extern void nrf_802154_rsch_delayed_timeslot_started(rsch_dly_ts_id_t dly_ts_id);

/**
 * @brief Notification that previously requested delayed timeslot cannot be started.
 *
 * This function may be called when any of radio activity precondition is not satisfied at the
 * time when the timeslot should start.
 *
 * @param[in]  dly_ts_id  Type of the failed timeslot.
 */
extern void nrf_802154_rsch_delayed_timeslot_failed(rsch_dly_ts_id_t dly_ts_id);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_RSCH_H_ */

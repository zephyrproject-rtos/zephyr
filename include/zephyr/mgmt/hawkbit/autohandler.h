/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief hawkBit autohandler header file
 */

/**
 * @brief hawkBit autohandler API.
 * @defgroup hawkbit_autohandler hawkBit autohandler API
 * @ingroup hawkbit
 * @{
 */

#ifndef ZEPHYR_INCLUDE_MGMT_HAWKBIT_AUTOHANDLER_H_
#define ZEPHYR_INCLUDE_MGMT_HAWKBIT_AUTOHANDLER_H_

#include <zephyr/mgmt/hawkbit/hawkbit.h>

/**
 * @brief Runs hawkBit probe and hawkBit update automatically
 *
 * @details The hawkbit_autohandler handles the whole process
 * in pre-determined time intervals.
 *
 * @param auto_reschedule If true, the handler will reschedule itself
 */
void hawkbit_autohandler(bool auto_reschedule);

/**
 * @brief Wait for the autohandler to finish.
 *
 * @param events Set of desired events on which to wait. Set to ::UINT32_MAX to wait for the
 *               autohandler to finish one run, or BIT() together with a value from
 *               ::hawkbit_response to wait for a specific event.
 * @param timeout Waiting period for the desired set of events or one of the
 *                special values ::K_NO_WAIT and ::K_FOREVER.
 *
 * @return A value from ::hawkbit_response.
 */
enum hawkbit_response hawkbit_autohandler_wait(uint32_t events, k_timeout_t timeout);

/**
 * @brief Cancel the run of the hawkBit autohandler.
 *
 * @return a value from k_work_cancel_delayable().
 */
int hawkbit_autohandler_cancel(void);

/**
 * @brief Set the delay for the next run of the autohandler.
 *
 * @details This function will only delay the next run of the autohandler. The delay will not
 * persist after the autohandler runs.
 *
 * @param timeout The delay to set.
 * @param if_bigger If true, the delay will be set only if the new delay is bigger than the current
 * one.
 *
 * @retval 0 if @a if_bigger was true and the current delay was bigger than the new one.
 * @retval otherwise, a value from k_work_reschedule().
 */
int hawkbit_autohandler_set_delay(k_timeout_t timeout, bool if_bigger);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_HAWKBIT_AUTOHANDLER_H_ */

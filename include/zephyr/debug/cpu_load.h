/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_CPU_LOAD_H_
#define ZEPHYR_INCLUDE_DEBUG_CPU_LOAD_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup cpu_load CPU load monitor
 *  @ingroup os_services
 *  @brief Module for monitoring CPU Load
 *
 *  This module allow monitoring of the CPU load.
 *  @{
 */

/** @brief Hook called by the application specific hook on entering CPU idle. */
void cpu_load_on_enter_idle(void);

/** @brief Hook called by the application specific hook on exiting CPU idle. */
void cpu_load_on_exit_idle(void);

/** @brief Get CPU load.
 *
 * CPU load is measured using a timer which tracks amount of time spent in the
 * CPU idle. Since it is a software tracking there is some small overhead.
 * Precision depends on the frequency of the timer in relation to the CPU frequency.
 *
 * @param reset Reset the measurement after reading.
 *
 * @retval Positive number - CPU load in per mille.
 * @retval Negative number - error code.
 */
int cpu_load_get(bool reset);

/** @brief Control periodic CPU statistics report.
 *
 * Report logging is by default enabled.
 *
 * @param enable true to enable report logging and false to disable.
 */
void cpu_load_log_control(bool enable);

/* Optional callback for cpu_load_cb_reg
 *
 * This will be called from the k_timer expiry_fn.
 * Time spent in this callback must be kept to a minimum.
 */
typedef void (*cpu_load_cb_t)(uint8_t percent);

/** @brief Optional registration of callback when load is greater or equal to the threshold.
 *
 * Submitting NULL pointer will cancel the callback.
 *
 * @retval 0 - Callback registered/cancelled.
 * @retval -EINVAL if the threshold is invalid.
 *
 */
int cpu_load_cb_reg(cpu_load_cb_t cb, uint8_t threshold_percent);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DEBUG_CPU_LOAD_H_ */

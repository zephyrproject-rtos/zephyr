/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_CPU_STATS_H_
#define ZEPHYR_INCLUDE_DEBUG_CPU_STATS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup cpu_stats CPU load monitor
 *  @ingroup os_services
 *  @brief Module for monitoring CPU Load
 *
 *  This module allow monitoring of the CPU load.
 *  @{
 */

/** @brief Hook called by the application specific hook on entering CPU idle.
 *
 * CONFIG_CPU_STATS_EXT_ON_ENTER_HOOK must be enabled as otherwise cpu_stats has
 * own implementation.
 */
void cpu_stats_on_enter_cpu_idle_hook(void);

/** @brief Hook called by the application specific hook on exiting CPU idle.
 *
 * CONFIG_CPU_STATS_EXT_ON_EXIT_HOOK must be enabled as otherwise cpu_stats has
 * own implementation.
 */
void cpu_stats_on_exit_cpu_idle_hook(void);

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
int cpu_stats_load_get(bool reset);

/** @brief Control periodic CPU statistics report.
 *
 * Report logging is by default enabled.
 *
 * @param enable true to enable report logging and false to disable.
 */
void cpu_stats_log_control(bool enable);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DEBUG_CPU_STATS_H_ */

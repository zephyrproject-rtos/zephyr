/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_CPU_LOAD_H_
#define ZEPHYR_SUBSYS_CPU_LOAD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CPU Load Monitoring
 * @defgroup subsys_cpu_load CPU Load
 * @since 4.3
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/**
 * @brief Get the CPU load as a percentage.
 *
 * Return the percent that the CPU has spent in the active (non-idle) state between calls to this
 * function.
 *
 * @param cpu_id The ID of the CPU for which to get the load.
 *
 * @retval CPU load in percent (0-100) in case of success
 * @retval -errno code in case of failure.
 *
 */
int cpu_load_get(int cpu_id);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_CPU_LOAD_H_ */

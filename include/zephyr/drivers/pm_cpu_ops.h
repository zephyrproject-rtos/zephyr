/*
 * Copyright 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PM_CPU_OPS_H_
#define ZEPHYR_INCLUDE_DRIVERS_PM_CPU_OPS_H_

/**
 * @file
 * @brief Public API for CPU Power Management
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup power_management_cpu_api CPU Power Management
 * @{
 */

/**
 * @brief Power down the calling core
 *
 * This call is intended for use in hotplug. A core that is powered down by
 * cpu_off can only be powered up again in response to a cpu_on
 *
 * @retval The call does not return when successful
 * @retval -ENOTSUP If the operation is not supported
 */
int pm_cpu_off(void);

/**
 * @brief Power up a core
 *
 * This call is used to power up cores that either have not yet been booted
 * into the calling supervisory software or have been previously powered down
 * with a cpu_off call
 *
 * @param cpuid CPU id to power on
 * @param entry_point Address at which the core must commence execution
 *
 * @retval 0 on success, a negative errno otherwise
 * @retval -ENOTSUP If the operation is not supported
 */
int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PM_CPU_OPS_H_ */

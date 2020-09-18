/*
 * Copyright (c) 2020 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_PM_CORE_H_
#define ZEPHYR_INCLUDE_POWER_PM_CORE_H_

#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize power management.
 *
 * Initialize power management every component
 * (e.g. pm_policy, platform_pm).
 */
void pm_init(void);

#ifdef __cplusplus
}
#endif

#endif

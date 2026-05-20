/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief [DEPRECATED] Backward-compatibility layer for <haltium_power.h>.
 *
 * This header has been renamed to <soc_power.h>. The file exists so that
 * out-of-tree code using the old include path keeps building during the
 * deprecation period. It will be removed in a future release.
 */

#ifndef ZEPHYR_SOC_NORDIC_COMMON_HALTIUM_POWER_H_
#define ZEPHYR_SOC_NORDIC_COMMON_HALTIUM_POWER_H_

#warning "<haltium_power.h> is deprecated, include <soc_power.h> instead"

#include <soc_power.h>

#endif /* ZEPHYR_SOC_NORDIC_COMMON_HALTIUM_POWER_H_ */

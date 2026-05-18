/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SL_CLOCK_MANAGER_EXT_FLASH_CONFIG_H
#define SL_CLOCK_MANAGER_EXT_FLASH_CONFIG_H

#include <zephyr/devicetree.h>

#if DT_NODE_HAS_PROP(DT_NODELABEL(extmem), clock_frequency)
#define SL_CLOCK_MANAGER_EXT_FLASH_MAX_FREQ DT_PROP(DT_NODELABEL(extmem), clock_frequency)
#endif

#endif /* SL_CLOCK_MANAGER_EXT_FLASH_CONFIG_H */

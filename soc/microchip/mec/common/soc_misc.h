/*
 * Copyright (c) 2025 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Microchip MEC MCU family memory mapped control register access
 *
 */

#ifndef _SOC_MICROCHIP_MEC_COMMON_SOC_MISC_H_
#define _SOC_MICROCHIP_MEC_COMMON_SOC_MISC_H_

#include <stdint.h>

/** @brief return 1 if eSPI TAF block is enabled else 0 */
int soc_taf_enabled(void);

/** @brief return core clock frequency in Hz */
uint32_t soc_core_clock_get(void);

#endif /* _SOC_MICROCHIP_MEC_COMMON_SOC_MISC_H_ */

/*
 * Copyright (C) 2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_MICROCHIP_SAM_COMMON_DELAY_H_
#define ZEPHYR_SOC_MICROCHIP_SAM_COMMON_DELAY_H_

#include <stdint.h>

void busy_delay(uint32_t cycle, uint32_t n);

#ifdef CONFIG_SOC_FAMILY_MICROCHIP_SAMA7
#define CYCLE_US 0x63U
#define CYCLE_MS 0x186C0U

#else
#error "Delay calibration is not defined for this SoC"
#endif

#define UDELAY(n) busy_delay(CYCLE_US, (uint32_t)(n))
#define MDELAY(n) busy_delay(CYCLE_MS, (uint32_t)(n))

#endif /* ZEPHYR_SOC_MICROCHIP_SAM_COMMON_DELAY_H_ */

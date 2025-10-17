/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_ARM_NORDIC_NRF_NRF54H_SOC_H_
#define SOC_ARM_NORDIC_NRF_NRF54H_SOC_H_

#include <soc_nrf_common.h>

#if defined(CONFIG_SOC_NRF54H20_CPUAPP)
#define RAMBLOCK_CONTROL_BIT_ICACHE MEMCONF_POWER_CONTROL_MEM1_Pos
#define RAMBLOCK_CONTROL_BIT_DCACHE MEMCONF_POWER_CONTROL_MEM2_Pos
#define RAMBLOCK_POWER_ID           0
#define RAMBLOCK_CONTROL_OFF        0
#define RAMBLOCK_RET_MASK           (MEMCONF_POWER_RET_MEM0_Msk)
#define RAMBLOCK_RET_BIT_ICACHE     MEMCONF_POWER_RET_MEM1_Pos
#define RAMBLOCK_RET_BIT_DCACHE     MEMCONF_POWER_RET_MEM2_Pos
#elif defined(CONFIG_SOC_NRF54H20_CPURAD)
#define RAMBLOCK_CONTROL_BIT_ICACHE MEMCONF_POWER_CONTROL_MEM6_Pos
#define RAMBLOCK_CONTROL_BIT_DCACHE MEMCONF_POWER_CONTROL_MEM7_Pos
#define RAMBLOCK_POWER_ID           0
#define RAMBLOCK_CONTROL_OFF        0
#define RAMBLOCK_RET_MASK                                                                          \
	(MEMCONF_POWER_RET_MEM0_Msk | MEMCONF_POWER_RET_MEM1_Msk | MEMCONF_POWER_RET_MEM2_Msk |    \
	 MEMCONF_POWER_RET_MEM3_Msk | MEMCONF_POWER_RET_MEM4_Msk | MEMCONF_POWER_RET_MEM5_Msk |    \
	 MEMCONF_POWER_RET_MEM8_Msk)
#define RAMBLOCK_RET2_MASK                                                                         \
	(MEMCONF_POWER_RET2_MEM0_Msk | MEMCONF_POWER_RET2_MEM1_Msk | MEMCONF_POWER_RET2_MEM2_Msk | \
	 MEMCONF_POWER_RET2_MEM3_Msk | MEMCONF_POWER_RET2_MEM4_Msk | MEMCONF_POWER_RET2_MEM5_Msk | \
	 MEMCONF_POWER_RET2_MEM8_Msk)
#define RAMBLOCK_RET_BIT_ICACHE  MEMCONF_POWER_RET_MEM6_Pos
#define RAMBLOCK_RET_BIT_DCACHE  MEMCONF_POWER_RET_MEM7_Pos
#define RAMBLOCK_RET2_BIT_ICACHE MEMCONF_POWER_RET2_MEM6_Pos
#define RAMBLOCK_RET2_BIT_DCACHE MEMCONF_POWER_RET2_MEM7_Pos
#endif

/**
 * @brief Enable or disable the retention for cache RAM blocks.
 *
 * @param enable True if the retention is to be enabled, false otherwise.
 */
void nrf_soc_memconf_retain_set(bool enable);

#endif /* SOC_ARM_NORDIC_NRF_NRF54H_SOC_H_ */

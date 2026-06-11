/*
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INFINEON_AURIX_TC4X_SOC_TOS_H_
#define ZEPHYR_SOC_INFINEON_AURIX_TC4X_SOC_TOS_H_

#ifndef _ASMLANGUAGE

#include <stdint.h>

enum aurix_tos {
	AURIX_TOS_CPU0 = 0x00U,
	AURIX_TOS_CPU1 = 0x01U,
	AURIX_TOS_CPU2 = 0x02U,
	AURIX_TOS_CPU3 = 0x03U,
	AURIX_TOS_CPU4 = 0x04U,
	AURIX_TOS_CPU5 = 0x05U,
	AURIX_TOS_CPUCS = 0x06U,
	AURIX_TOS_DMA0 = 0x07U,
	AURIX_TOS_DMA1 = 0x08U,
	AURIX_TOS_GTM = 0x09U,
	AURIX_TOS_PPU = 0x0AU,
	AURIX_TOS_RDMA = 0x0BU,
};

static inline enum aurix_tos aurix_coreid_to_tos(uint32_t core_id)
{
	return core_id;
}

#endif
#endif /* ZEPHYR_SOC_INFINEON_AURIX_TC4X_SOC_TOS_H_ */
